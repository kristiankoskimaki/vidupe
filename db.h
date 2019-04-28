#ifndef DB_H
#define DB_H

#include "video.h"
#include <QSqlDatabase>

class Db
{

public:
    Db(const QString &filename);
    ~Db() { _db.close(); _db = QSqlDatabase(); _db.removeDatabase(_id); }

    QSqlDatabase _db;
    QString _id = "default";

public slots:
    QString uniqueId(const QString &filename="") const;
    bool read(Video &video) const;        //return true if video is cached
    void write(const Video &video) const;
};

#endif // DB_H
