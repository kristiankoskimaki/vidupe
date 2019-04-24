#ifndef DB_H
#define DB_H

#include "video.h"
#include <QSqlDatabase>

class Db
{

public:
    Db(const QString &id);
    ~Db() { _db.close(); _db = QSqlDatabase(); _db.removeDatabase(_id); }

    QSqlDatabase _db;
    QString _id;

    bool read(Video &video) const;        //return true if video is cached
    void write(Video &video) const;
};

#endif // DB_H
