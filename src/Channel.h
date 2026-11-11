#ifndef CHANNEL_H
#define CHANNEL_H

#include <QString>
#include <QJsonObject>
#include <QList>
#include <QTcpSocket>

class Channel
{
private:
    QString channelID;
    QString name;

    QList<QTcpSocket*> members;
    QList<QJsonObject> messageHistory;

    static constexpr int MAX_HISTORY = 100;

public:
    Channel(
        const QString& id,
        const QString& channelName
    );

    // Getters
    QString getChannelID() const;
    QString getName() const;

    // Channel management
    void setName(const QString& newName);

    bool addMember(QTcpSocket* socket);
    void removeMember(QTcpSocket* socket);

    bool hasMember(QTcpSocket* socket) const;

    int getMemberCount() const;

    // Messaging
    void broadcast(const QJsonObject& message);
    void addMessage(const QJsonObject& message);

    QList<QJsonObject> getMessageHistory() const;
};

#endif
