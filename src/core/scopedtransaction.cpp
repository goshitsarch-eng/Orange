#include "core/scopedtransaction.h"

#include "core/database.h"

ScopedTransaction::ScopedTransaction(Database *db) : db_(db), pending_(false) {
  if (db_ && db_->Exec("BEGIN")) {
    pending_ = true;
  }
}

ScopedTransaction::~ScopedTransaction() {
  if (pending_ && db_) {
    db_->Exec("ROLLBACK");
  }
}

void ScopedTransaction::Commit() {
  if (pending_ && db_ && db_->Exec("COMMIT")) {
    pending_ = false;
  }
}
