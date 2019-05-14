#ifndef DB_H
#define DB_H

#include <QSqlDatabase>
#include <QDateTime>

class Video;

class Db
{

public:
    Db(const QString &filename);
    ~Db() { _db.close(); _db = QSqlDatabase(); _db.removeDatabase(_connection); }

    QSqlDatabase _db;
    QString _connection;
    QString _id;
    QDateTime _modified;

public:
    QString uniqueId(const QString &filename="") const;
    void createTables() const;
    bool readMetadata(Video &video) const;        //return true if video is cached
    void writeMetadata(const Video &video) const;
    QByteArray readCapture(const int &percent) const;
    void writeCapture(const int &percent, const QByteArray &image) const;

    //removeVideo returns false if id not cached or could not be removed
    bool removeVideo(const QString &id) const;
};

#endif // DB_H
