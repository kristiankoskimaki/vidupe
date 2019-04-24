#include "db.h"
#include <QApplication>
#include <QSqlQuery>

Db::Db(const QString &id)
{
    _id = id;
    const QString dbfilename = QString("%1/cache.db").arg(QApplication::applicationDirPath());
    _db = QSqlDatabase::addDatabase("QSQLITE", _id);
    _db.setDatabaseName(dbfilename);
    _db.open();

    const QFileInfo dbFile(dbfilename);     //create a cache database if one does not exist
    if(dbFile.size() == 0)
    {
        QSqlQuery query(_db);
        query.exec("CREATE TABLE videos (id TEXT PRIMARY KEY, "
                   "size INTEGER, duration INTEGER, bitrate INTEGER, framerate REAL, "
                   "codec TEXT, audio TEXT, width INTEGER, height INTEGER)");
    }
}

bool Db::read(Video &video) const
{
    QSqlQuery query(_db);
    query.exec(QString("SELECT * FROM videos WHERE id = '%1'").arg(_id));

    while(query.next())
    {
        video.size = query.value(1).toLongLong();
        video.duration = query.value(2).toLongLong();
        video.bitrate = query.value(3).toUInt();
        video.framerate = query.value(4).toDouble();
        video.codec = query.value(5).toString();
        video.audio = query.value(6).toString();
        video.width = static_cast<ushort>(query.value(7).toUInt());
        video.height = static_cast<ushort>(query.value(8).toUInt());
        return true;
    }
    return false;
}

void Db::write(Video &video) const
{
    QSqlQuery query(_db);
    query.exec(QString("INSERT INTO videos VALUES('%1',%2,%3,%4,%5,'%6','%7',%8,%9)")
               .arg(_id).arg(video.size).arg(video.duration).arg(video.bitrate).arg(video.framerate)
               .arg(video.codec).arg(video.audio).arg(video.width).arg(video.height));
}
