#ifndef STRAWBERRY_ENGINEPIPELINEFINISH_H
#define STRAWBERRY_ENGINEPIPELINEFINISH_H

namespace EnginePipelineFinish {

// Qt GstEnginePipeline::Finish: already NULL and idle means done immediately.
inline bool AlreadyFinished(bool has_element, bool is_null, bool change_in_progress) {
  return !has_element || (is_null && !change_in_progress);
}

inline bool ChangeInProgress(bool async_return, bool has_pending) { return async_return || has_pending; }

// Qt FinishPipeline: keep the pipeline in old_pipelines_ until Finished.
inline bool KeepUntilFinished(bool finish_returned) { return !finish_returned; }

inline bool ShouldEmitIdleFinished(bool finish_requested, bool already_emitted, bool reached_null) {
  return finish_requested && reached_null && !already_emitted;
}

}  // namespace EnginePipelineFinish

#endif
