use orange_app::library::CollectionState;
use std::sync::{atomic::AtomicBool, Arc};

#[test]
fn failed_or_cancelled_rescan_preserves_the_last_complete_index() {
    let root = tempfile::tempdir().unwrap();
    let music = root.path().join("Música 日本");
    std::fs::create_dir(&music).unwrap();
    std::fs::write(music.join("01 Track.wav"), b"metadata fallback fixture").unwrap();
    let mut collection = CollectionState {
        db_path: root.path().join("orange.db"),
        ..CollectionState::default()
    };
    collection.add_folder(music.to_str().unwrap()).unwrap();
    assert_eq!(collection.songs.len(), 1);
    collection.cancellation = Some(Arc::new(AtomicBool::new(true)));
    assert!(collection.rescan_all().is_err());
    collection.cancellation = None;
    collection.reload();
    assert_eq!(collection.songs.len(), 1);
    std::fs::rename(&music, root.path().join("unmounted")).unwrap();
    assert!(collection.rescan_all().is_err());
    collection.reload();
    assert_eq!(collection.songs.len(), 1);
}
