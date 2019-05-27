#ifndef DB_H
#define DB_H

#include <QSqlDatabase>
#include <QDateTime>

class Video;

class Db
{

public:
    explicit Db(const QString &filename);
    ~Db() { _db.close(); _db = QSqlDatabase(); _db.removeDatabase(_connection); }

private:
    QSqlDatabase _db;
    QString _connection;
    QString _id;
    QDateTime _modified;

public:
    //return md5 hash of parameter's file, or (as convinience) md5 hash of the file given to constructor
    QString uniqueId(const QString &filename=QStringLiteral("")) const;

    //constructor creates a database file if there is none already
    void createTables() const;

    //return true and updates member variables if the video metadata was cached
    bool readMetadata(Video &video) const;

    //save video properties in cache
    void writeMetadata(const Video &video) const;

    //returns screen capture if it was cached, else return null ptr
    QByteArray readCapture(const int &percent) const;

    //save image in cache
    void writeCapture(const int &percent, const QByteArray &image) const;

    //returns false if id not cached or could not be removed
    bool removeVideo(const QString &id) const;
};

#endif // DB_H
