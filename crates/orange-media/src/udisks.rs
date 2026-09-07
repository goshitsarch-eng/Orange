//! UDisks2 drive enumeration (feature `dbus`).
//!
//! Finds optical drives (Audio CD source) and mounted removable volumes
//! (USB mass-storage sync targets) via `org.freedesktop.UDisks2`. The D-Bus
//! reply parses into plain structs through [`parse_managed_objects`], which
//! is unit-tested with fixture maps; only the transport needs a bus.

use std::collections::HashMap;

use zbus::zvariant::{Array, OwnedObjectPath, OwnedValue};
use zbus::{Connection, Proxy};

/// Well-known UDisks2 bus name and root path.
pub const UDISKS_BUS: &str = "org.freedesktop.UDisks2";
pub const UDISKS_PATH: &str = "/org/freedesktop/UDisks2";

/// An optical drive (Audio CD reads via `cdda://` URIs).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct OpticalDrive {
    /// Kernel node, e.g. `/dev/sr0`.
    pub device_node: String,
}

/// A mounted removable volume (device sync target).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RemovableVolume {
    /// Kernel node, e.g. `/dev/sdb1`.
    pub device_node: String,
    /// Mount point, e.g. `/run/media/gosh/WALKMAN`.
    pub mount_path: String,
    /// Filesystem label (may be empty).
    pub label: String,
}

/// Raw `GetManagedObjects` reply shape.
pub type ManagedObjects = HashMap<OwnedObjectPath, HashMap<String, HashMap<String, OwnedValue>>>;

fn interface_props<'a>(
    objects: &'a ManagedObjects,
    path: &OwnedObjectPath,
    interface: &str,
) -> Option<&'a HashMap<String, OwnedValue>> {
    objects.get(path)?.get(interface)
}

fn prop_string(props: &HashMap<String, OwnedValue>, key: &str) -> Option<String> {
    props
        .get(key)
        .and_then(|value| String::try_from(value.clone()).ok())
}

fn prop_bool(props: &HashMap<String, OwnedValue>, key: &str) -> bool {
    props
        .get(key)
        .and_then(|value| bool::try_from(value.clone()).ok())
        .unwrap_or(false)
}

/// Decode a D-Bus `ay` (NUL-terminated byte path) to a Rust string.
fn byte_array_to_path(value: &OwnedValue) -> Option<String> {
    let array = Array::try_from(value.clone()).ok()?;
    let bytes: Vec<u8> = array
        .iter()
        .filter_map(|item| u8::try_from(item.clone()).ok())
        .collect();
    let end = bytes
        .iter()
        .position(|&byte| byte == 0)
        .unwrap_or(bytes.len());
    String::from_utf8(bytes[..end].to_vec()).ok()
}

/// First mounted path from a UDisks2 `MountPoints` (`aay`) value.
fn first_mount_point(value: &OwnedValue) -> Option<String> {
    let outer = Array::try_from(value.clone()).ok()?;
    let inner = outer.iter().next()?;
    let inner = Array::try_from(inner.clone()).ok()?;
    let bytes: Vec<u8> = inner
        .iter()
        .filter_map(|item| u8::try_from(item.clone()).ok())
        .collect();
    byte_array_to_path(&owned_bytes(&bytes))
}

/// True D-Bus `ay`: element type byte, not variant.
fn owned_bytes(bytes: &[u8]) -> OwnedValue {
    use zbus::zvariant::Value;
    OwnedValue::try_from(Value::Array(Array::from(bytes.to_vec()))).expect("bytes to value")
}

/// Split managed objects into optical drives and mounted removable volumes.
///
/// A block object counts as removable when it has a `Filesystem` interface
/// with at least one mount point and is not the system root: hint
/// `System=false` on the Block interface, or a mount under `/run/media` or
/// `/media`. Optical media itself is never a sync target.
pub fn parse_managed_objects(
    objects: &ManagedObjects,
) -> (Vec<OpticalDrive>, Vec<RemovableVolume>) {
    let mut drives = Vec::new();
    let mut volumes = Vec::new();
    for (path, _interfaces) in objects {
        let Some(block) = interface_props(objects, path, "org.freedesktop.UDisks2.Block") else {
            continue;
        };
        let Some(device_node) = block
            .get("Device")
            .and_then(byte_array_to_path)
            .filter(|node| !node.is_empty() && node != "/dev/null")
        else {
            continue;
        };
        if prop_bool(block, "Optical") {
            drives.push(OpticalDrive { device_node });
            continue;
        }
        let Some(filesystem) = interface_props(objects, path, "org.freedesktop.UDisks2.Filesystem")
        else {
            continue;
        };
        let Some(mounts) = filesystem.get("MountPoints") else {
            continue;
        };
        let Some(mount_path) = first_mount_point(mounts).filter(|mount| !mount.is_empty()) else {
            continue;
        };
        let system_hint = prop_bool(block, "HintSystem");
        let removable_mount =
            mount_path.starts_with("/run/media/") || mount_path.starts_with("/media/");
        if system_hint && !removable_mount {
            continue;
        }
        volumes.push(RemovableVolume {
            device_node,
            mount_path,
            label: prop_string(block, "IdLabel").unwrap_or_default(),
        });
    }
    drives.sort_by(|a, b| a.device_node.cmp(&b.device_node));
    volumes.sort_by(|a, b| a.mount_path.cmp(&b.mount_path));
    (drives, volumes)
}

/// Query live managed objects from the session bus.
pub async fn query_managed_objects(conn: &Connection) -> Result<ManagedObjects, String> {
    let proxy = Proxy::new(
        conn,
        UDISKS_BUS,
        UDISKS_PATH,
        "org.freedesktop.DBus.ObjectManager",
    )
    .await
    .map_err(|e| e.to_string())?;
    let reply = proxy
        .call_method("GetManagedObjects", &())
        .await
        .map_err(|e| e.to_string())?;
    reply
        .body()
        .deserialize::<ManagedObjects>()
        .map_err(|e| e.to_string())
}

/// Convenience: optical drives plus removable volumes from the live bus.
pub async fn enumerate_devices(
    conn: &Connection,
) -> Result<(Vec<OpticalDrive>, Vec<RemovableVolume>), String> {
    Ok(parse_managed_objects(&query_managed_objects(conn).await?))
}

#[cfg(test)]
mod tests {
    use super::*;
    use zbus::zvariant::{ObjectPath, Value};

    fn object_path(value: &str) -> OwnedObjectPath {
        OwnedObjectPath::try_from(value).expect("fixture object path")
    }

    fn owned_str(value: &str) -> OwnedValue {
        OwnedValue::try_from(Value::from(value.to_string())).expect("str to value")
    }

    fn owned_bool(value: bool) -> OwnedValue {
        OwnedValue::try_from(Value::from(value)).expect("bool to value")
    }

    fn device_bytes(node: &str) -> OwnedValue {
        let mut bytes = node.as_bytes().to_vec();
        bytes.push(0);
        owned_bytes(&bytes)
    }

    /// True D-Bus `aay`: an array of byte arrays (not variants).
    fn mounts(paths: &[&str]) -> OwnedValue {
        use zbus::zvariant::{Signature, Value};
        let element: Signature = "ay".parse().expect("ay signature");
        let mut outer = Array::new(&element);
        for path in paths {
            let mut bytes = path.as_bytes().to_vec();
            bytes.push(0);
            outer
                .append(Value::Array(Array::from(bytes)))
                .expect("append mount");
        }
        OwnedValue::try_from(Value::Array(outer)).expect("mounts to value")
    }

    fn block_object(
        object: &str,
        device: &str,
        optical: bool,
        system: bool,
        label: &str,
        mounts: Option<OwnedValue>,
    ) -> (
        OwnedObjectPath,
        HashMap<String, HashMap<String, OwnedValue>>,
    ) {
        let mut block = HashMap::new();
        block.insert("Device".to_string(), device_bytes(device));
        block.insert("Optical".to_string(), owned_bool(optical));
        block.insert("HintSystem".to_string(), owned_bool(system));
        block.insert("IdLabel".to_string(), owned_str(label));
        let mut interfaces = HashMap::new();
        interfaces.insert("org.freedesktop.UDisks2.Block".to_string(), block);
        if let Some(mounts) = mounts {
            let mut filesystem = HashMap::new();
            filesystem.insert("MountPoints".to_string(), mounts);
            interfaces.insert("org.freedesktop.UDisks2.Filesystem".to_string(), filesystem);
        }
        (object_path(object), interfaces)
    }

    #[test]
    fn parses_optical_and_removable() {
        let mut objects = ManagedObjects::new();
        let (cd_path, cd) = block_object(
            "/org/freedesktop/UDisks2/block_devices/sr0",
            "/dev/sr0",
            true,
            false,
            "",
            None,
        );
        objects.insert(cd_path, cd);
        let (usb_path, usb) = block_object(
            "/org/freedesktop/UDisks2/block_devices/sdb1",
            "/dev/sdb1",
            false,
            false,
            "WALKMAN",
            Some(mounts(&["/run/media/gosh/WALKMAN"])),
        );
        objects.insert(usb_path, usb);
        let (drives, volumes) = parse_managed_objects(&objects);
        assert_eq!(
            drives,
            vec![OpticalDrive {
                device_node: "/dev/sr0".to_string()
            }]
        );
        assert_eq!(volumes.len(), 1);
        assert_eq!(volumes[0].mount_path, "/run/media/gosh/WALKMAN");
        assert_eq!(volumes[0].label, "WALKMAN");
    }

    #[test]
    fn skips_system_mounts_and_unmounted() {
        let mut objects = ManagedObjects::new();
        let (root_path, root) = block_object(
            "/org/freedesktop/UDisks2/block_devices/sda2",
            "/dev/sda2",
            false,
            true,
            "",
            Some(mounts(&["/"])),
        );
        objects.insert(root_path, root);
        let (bare_path, bare) = block_object(
            "/org/freedesktop/UDisks2/block_devices/sdc1",
            "/dev/sdc1",
            false,
            false,
            "",
            None,
        );
        objects.insert(bare_path, bare);
        let (drives, volumes) = parse_managed_objects(&objects);
        assert!(drives.is_empty());
        assert!(volumes.is_empty());
    }

    #[test]
    fn byte_paths_decode() {
        assert_eq!(
            byte_array_to_path(&device_bytes("/dev/sr0")),
            Some("/dev/sr0".to_string())
        );
        assert_eq!(
            first_mount_point(&mounts(&["/media/usb", "/media/usb2"])),
            Some("/media/usb".to_string())
        );
    }

    #[test]
    fn object_path_helper_validates() {
        assert!(ObjectPath::try_from("/org/mpris/MediaPlayer2").is_ok());
    }
}
