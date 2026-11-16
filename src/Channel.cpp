#include "Channel.h"

#include <QJsonDocument>
#include <QAbstractSocket>

Channel::Channel(
    const QString& id,
    const QString& channelName
)
    : channelID(id),
      name(channelName)
{
}

// Getters

QString Channel::getChannelID() const
{
    return channelID;
}

QString Channel::getName() const
{
    return name;
}

// Rename channel

void Channel::setName(const QString& newName)
{
    if (!newName.trimmed().isEmpty())
    {
        name = newName.trimmed();
    }
}

// Member management

bool Channel::addMember(QTcpSocket* socket)
{
    if (socket == nullptr ||
        socket->state() != QAbstractSocket::ConnectedState ||
        hasMember(socket))
    {
        return false;
    }

    members.append(socket);
    return true;
}

void Channel::removeMember(QTcpSocket* socket)
{
    members.removeAll(socket);
}

bool Channel::hasMember(QTcpSocket* socket) const
{
    return members.contains(socket);
}

int Channel::getMemberCount() const
{
    return members.size();
}

// Message history

void Channel::addMessage(const QJsonObject& message)
{
    messageHistory.append(message);

    while (messageHistory.size() > MAX_HISTORY)
    {
        messageHistory.removeFirst();
    }
}

QList<QJsonObject> Channel::getMessageHistory() const
{
    return messageHistory;
}

// Broadcast a message to channel members

void Channel::broadcast(const QJsonObject& message)
{
    QByteArray data =
        QJsonDocument(message).toJson(QJsonDocument::Compact)
        + '\n';

    for (auto it = members.begin(); it != members.end();)
    {
        QTcpSocket* socket = *it;

        if (socket == nullptr ||
            socket->state() != QAbstractSocket::ConnectedState)
        {
            it = members.erase(it);
            continue;
        }

        if (socket->bytesToWrite() > 256 * 1024)
        {
            socket->disconnectFromHost();
            it = members.erase(it);
            continue;
        }

        socket->write(data);
        ++it;
    }
}
