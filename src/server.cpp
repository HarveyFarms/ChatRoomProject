

#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHash>
#include <QRegularExpression>
#include <QTimer>
#include <QTextStream>

#include <exception>

#include "Authentication.h"
#include "User.h"

class ChatServer : public QObject {
  QTcpServer listener;
  QHash<QTcpSocket*, QString> clients;
  Authentication authentication;
  QHash<QTcpSocket*, User*> authenticatedUsers;

  static constexpr qint64 maxFrame = 8192;

  void log(const QString &s) {
    QTextStream(stdout) << s << Qt::endl;
  }

  void send(QTcpSocket *s, const QJsonObject &message) {
    if (s->state() != QAbstractSocket::ConnectedState) return;

    if (s->bytesToWrite() > 256 * 1024) {
      s->disconnectFromHost();
      return;
    }

    s->write(
        QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n'
    );
  }

  void broadcast(const QJsonObject &message) {
    const auto sockets = clients.keys();

    for (auto *s : sockets) {
      if (!clients.value(s).isEmpty()) {
        send(s, message);
      }
    }
  }

  QJsonObject roster() const {
    QStringList names;

    for (const auto &name : clients) {
      if (!name.isEmpty()) names.append(name);
    }

    names.sort(Qt::CaseInsensitive);

    return {
      {"type", "users"},
      {"names", QJsonArray::fromStringList(names)}
    };
  }

  void announceRoster() {
    auto r = roster();
    broadcast(r);

    QStringList names;

    for (const auto &v : r["names"].toArray()) {
      names.append(v.toString());
    }

    log(QString("Online (%1): %2")
        .arg(names.size())
        .arg(names.join(", ")));
  }

  void error(QTcpSocket *s, const QString &message) {
    send(s, {{"type", "error"}, {"text", message}});
  }

  void handle(QTcpSocket *s, const QJsonObject &m) {
    const QString type = m["type"].toString();

    // Registration and login are available before authentication.
    if (!authenticatedUsers.contains(s)) {

      if (type == "register") {
        QString name = m["username"].toString();
        QString email = m["email"].toString();
        QString password = m["password"].toString();

        if (!QRegularExpression(
                "^[A-Za-z0-9_-]{1,24}$").match(name).hasMatch()) {
          error(s, "Invalid username.");
          return;
        }

        if (email.trimmed().isEmpty() ||
            password.isEmpty() || password.size() > 1024) {
          error(s, "Email and password are required.");
          return;
        }

        try {
          User *user = authentication.registerUser(
              name.toStdString(),
              email.toStdString(),
              password.toStdString()
          );

          if (user == nullptr) {
            error(s, "Username or email is already registered.");
            return;
          }

          send(s, {
              {"type", "notice"},
              {"text", "Registration successful. Please log in."}
          });

          log("REGISTER " + name);
        } catch (const std::exception &e) {
          log(QString("Registration failed: %1").arg(e.what()));
          error(s, "Registration failed.");
        }

        return;
      }

      if (type == "login") {
        QString name = m["username"].toString();
        QString password = m["password"].toString();

        if (name.isEmpty() || password.isEmpty() ||
            password.size() > 1024) {
          error(s, "Username and password are required.");
          return;
        }

        // Find and verify the account before changing its online status.
        User *candidate =
            authentication.findUserByUsername(name.toStdString());

        if (candidate == nullptr ||
            candidate->isBanned() ||
            !candidate->verifyPassword(password.toStdString())) {
          error(s, "Invalid username or password.");
          return;
        }

        // Do not allow an account to log in from two sockets.
        for (auto it = authenticatedUsers.cbegin();
             it != authenticatedUsers.cend(); ++it) {
          if (it.value() == candidate) {
            error(s, "This account is already connected.");
            return;
          }
        }

        // login() verifies the password and marks the account online.
        User *user = authentication.login(
            name.toStdString(),
            password.toStdString()
        );

        if (user == nullptr) {
          error(s, "Invalid username or password.");
          return;
        }

        authenticatedUsers.insert(s, user);
        clients[s] = QString::fromStdString(user->getUsername());

        const QString loggedInName = clients.value(s);

        send(s, {
            {"type", "welcome"},
            {"name", loggedInName},
            {"text", "Login successful. Welcome to the lobby."}
        });

        broadcast({
            {"type", "notice"},
            {"text", loggedInName + " joined the server."}
        });

        log("LOGIN " + loggedInName +
            " from " + s->peerAddress().toString());

        announceRoster();
        return;
      }

      error(s, "Please register or log in first.");
      return;
    }

    // The socket is authenticated from this point onward.
    if (type == "users") {
      send(s, roster());
    } else if (type == "chat") {
      QString message = m["text"].toString().trimmed();

      if (message.isEmpty() || message.size() > 2000 ||
          message.contains('\n') || message.contains('\r')) {
        error(s, "Send a single line of 1-2000 characters.");
        return;
      }

      broadcast({
          {"type", "chat"},
          {"name", clients.value(s)},
          {"text", message}
      });
    } else {
      error(s, "Unknown command.");
    }
  }

  void receive(QTcpSocket *s) {
    while (s->canReadLine()) {
      QByteArray line = s->readLine(maxFrame + 1);

      if (!line.endsWith('\n') || line.size() > maxFrame) {
        error(s, "Message too large.");
        s->disconnectFromHost();
        return;
      }

      QJsonParseError parse;
      auto doc = QJsonDocument::fromJson(line, &parse);

      if (parse.error != QJsonParseError::NoError ||
          !doc.isObject()) {
        error(s, "Expected a JSON object.");
        continue;
      }

      handle(s, doc.object());

      if (s->state() != QAbstractSocket::ConnectedState) return;
    }

    if (s->bytesAvailable() >= maxFrame) {
      error(s, "Message too large.");
      s->disconnectFromHost();
    }
  }

public:
  ChatServer() {
    connect(&listener, &QTcpServer::newConnection, this, [this] {
      while (listener.hasPendingConnections()) {
        auto *s = listener.nextPendingConnection();

        if (clients.size() >= 100) {
          s->disconnectFromHost();
          s->deleteLater();
          continue;
        }

        clients.insert(s, QString());
        s->setReadBufferSize(16384);

        connect(s, &QTcpSocket::readyRead, this,
                [this, s] { receive(s); });

        connect(s, &QTcpSocket::disconnected, this, [this, s] {
          // Authentication owns User objects; do not delete them here.
          User *user = authenticatedUsers.take(s);

          if (user != nullptr) {
            authentication.logout(user);
          }

          const QString name = clients.take(s);

          if (!name.isEmpty()) {
            log("LEAVE " + name);

            broadcast({
                {"type", "notice"},
                {"text", name + " left the server."}
            });

            announceRoster();
          }

          s->deleteLater();
        });

        // Require a login or registration attempt within 30 seconds.
        QTimer::singleShot(30000, s, [this, s] {
          if (clients.contains(s) &&
              clients.value(s).isEmpty()) {
            s->disconnectFromHost();
          }
        });
      }
    });
  }

  bool start(const QHostAddress &host, quint16 port) {
    if (!listener.listen(host, port)) {
      log("Cannot listen: " + listener.errorString());
      return false;
    }

    log(QString("Listening on %1:%2")
        .arg(host.toString())
        .arg(listener.serverPort()));

    log("Online (0). Ctrl+C stops the server.");
    return true;
  }
};

int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);

  const auto args = app.arguments();
  bool ok = true;

  int port = args.size() > 1 ? args[1].toInt(&ok) : 5050;
  QHostAddress host(args.size() > 2 ? args[2] : "127.0.0.1");

  if (!ok || port < 1 || port > 65535 ||
      host.isNull() || args.size() > 3) {
    QTextStream(stderr)
        << "Usage: chat_server [port=5050] "
           "[bind-address=127.0.0.1]\n";
    return 1;
  }

  ChatServer server;

  if (!server.start(host, static_cast<quint16>(port))) return 1;

  return app.exec();
}
