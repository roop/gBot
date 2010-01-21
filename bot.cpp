#include "bot.h"


QString Bot::commit(const QString &arg)
{
	QStringList arguments;
	
	QString commitMessage = arg;
	commitMessage.replace(QChar('\n'),"\n PRIVMSG "+channel+" :");
	commitMessage = "PRIVMSG "+channel+" :" + commitMessage + "\n";
	sendMessage(commitMessage);
	commitMessage.clear();
    return QString("commit(\"%1\") got called").arg(arg);
}

void Bot::onConnect()
{
	connect(&tcpSocket, SIGNAL(readyRead()), this, SLOT(onReadReady()));
    dataToSend = "NICK " + nick + "\r\n" + "USER " + iDent + ' ' + serverAddress +" bla :"+ realName  +"\r\n";
    sendMessage(dataToSend);
}

void Bot::onError( QAbstractSocket::SocketError socketError )
{

	qDebug() <<"There was a socket error";

}

QString Bot::serviceName()
{
	return DBusInterface;
}

Bot::Bot( const QString &configurationFileName )
{
	QSettings settings( configurationFileName, QSettings::IniFormat );
	serverAddress = settings.value("serverAddress").toString();
	nick = settings.value("nick").toString();
	realName = settings.value("realName").toString();
	port = settings.value("port").toInt();
	iDent = settings.value("iDent").toString();
	owner = settings.value("owner").toString();
	channel = settings.value("channel").toString();
	DBusInterface = settings.value("DBusInterface").toString();
	tcpSocket.abort();
    tcpSocket.connectToHost( serverAddress, port);
	connect(&tcpSocket ,SIGNAL(connected()), this, SLOT(onConnect()));
	connect(&tcpSocket ,SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));

}


void Bot::sendMessage(const QString &messageBlock)
{
	qDebug() << messageBlock; 
    
    qint64 i = tcpSocket.write(messageBlock.toAscii());            	
	tcpSocket.flush();
}

void Bot::sendHelp()
{
	QDir dir;
	QString extension;
	QString commandsAvaliable;
    dir.setFilter(QDir::Files);
    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
		extension = fileInfo.suffix();
		if ( extension == "js" )
		{ commandsAvaliable.append("!"+fileInfo.fileName().split(".")[0]+"  ");}
	}
	sendMessage("PRIVMSG "+channel+" :The following commands are available: "+commandsAvaliable+"\n");
}

static QScriptValue Foo(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue a = context->argument(0);
    QScriptValue b = context->argument(1);
    return "haha";
}

void Bot::executeScript(const QString &scriptName)
{
	QScriptEngine *engine = new QScriptEngine;
	QString fullScriptName = scriptName;
	
	QScriptValue fooProto = engine->newObject();
	engine->globalObject().setProperty("Foo", engine->newFunction(Foo));
	
	fullScriptName.replace(QChar('\n'),".js");
    QFile script( fullScriptName );
    script.open(QFile::ReadOnly);
    QString code = script.readAll();
    QScriptValue value = engine->evaluate(code);
	sendMessage("PRIVMSG "+channel+" :"+ value.toString() + "\n");
}

void Bot::onReadReady()
{
	QByteArray reply;
	QString serverThatPinged;
	QString scriptName;
	while(tcpSocket.canReadLine())
	{
		reply = tcpSocket.readLine();
		qDebug() << reply;
		if ( reply.indexOf("identd") >= 0)
		{
			sendMessage("JOIN "+channel+"\n");
		}
		if ( reply.indexOf("PRIVMSG") >= 0 && reply.indexOf(nick) >= 0)
		{
			if ( reply.indexOf("ping") >= 0)
			{
				sendMessage("PRIVMSG "+channel+" :PONG!\n");		
			}else 
			if ( reply.indexOf("PRIVMSG "+channel+" :"+nick+", !") >= 0 || reply.indexOf("PRIVMSG "+channel+" :"+nick+": !") >= 0 )
			{
				if ( reply.indexOf("!help") > 0 )  
				{
					sendHelp();	
				}
				else
				{
                	scriptName = reply.split('!')[2].split('\0')[0];
					int length = scriptName.size();
					scriptName.replace(length-2,3,".js");
                	executeScript(scriptName);
				}
			}else		
			
			{
				sendMessage("PRIVMSG "+channel+" :I do not just print commit messages!\n");
			}
		}
		if ( reply.indexOf("PING") == 0 )
		{
			serverThatPinged = reply.split(':')[1];
			sendMessage("PONG :"+serverThatPinged);
		}
	}
}
