#ifndef STRAWBERRY_SCOPEDTRANSACTION_H
#define STRAWBERRY_SCOPEDTRANSACTION_H

class Database;

class ScopedTransaction {
 public:
  explicit ScopedTransaction(Database *db);
  ~ScopedTransaction();

  void Commit();

 private:
  Database *db_ = nullptr;
  bool pending_ = false;
};

#endif
