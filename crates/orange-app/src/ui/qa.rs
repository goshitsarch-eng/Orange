//! Opt-in native WebView automation. Excluded from ordinary release builds.
use super::*;

pub(super) fn install(_context: Context) {
    let handle = use_context::<Handle>();
    use_future(move || {
        let handle = handle.clone();
        async move {
            let Ok(script_path) = std::env::var("ORANGE_UI_QA_SCRIPT") else {
                return;
            };
            tokio::time::sleep(std::time::Duration::from_secs(2)).await;
            let result = match std::fs::read_to_string(script_path) {
                Ok(script) => document::eval(&script).await.map_err(|e| e.to_string()),
                Err(error) => Err(error.to_string()),
            };
            if let Ok(path) = std::env::var("ORANGE_UI_QA_RESULT") {
                if let Err(error) = std::fs::write(
                    path,
                    serde_json::to_string(&match result {
                        Ok(ref value) => value.clone(),
                        Err(ref error) => serde_json::json!({"ok":false,"error":error}),
                    })
                    .unwrap_or_else(|e| e.to_string()),
                ) {
                    tracing::error!("Cannot write UI QA result: {error}");
                }
            }
            if std::env::var("ORANGE_UI_QA_EXIT").as_deref() == Ok("1") {
                let passed = result
                    .as_ref()
                    .ok()
                    .and_then(|value| value.get("ok"))
                    .and_then(serde_json::Value::as_bool)
                    == Some(true);
                if let Err(error) = handle.shutdown(std::time::Duration::from_secs(2)) {
                    tracing::error!("UI QA shutdown failed: {error}");
                    std::process::exit(1);
                }
                std::process::exit(if passed { 0 } else { 1 });
            }
        }
    });
}
