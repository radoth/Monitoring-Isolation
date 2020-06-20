#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QUrl>
#include <QList>
class QFile;
class QNetworkReply;
class QNetworkAccessManager;

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    void startRequest(QUrl url);
    QString downloadfolder(QString dir);

private:
    Ui::Widget *ui;
    QNetworkAccessManager *manager;
    QNetworkReply *reply;
    QUrl url;   //网络地址
    QFile *file;    //文件指针
    QStringList good,bad;
    int stranger,know;
    int downCount=1;
    int goodPtr=0;
    int badPtr=0;
    void downControlA();
    void prepareList();
    void analysisData();
    bool updateUI;

private slots:
    void httpFinished();    //完成下载后的处理动作
    void httpReadyRead();   //接受数据时的处理动作
    void updateDataReadProgress(qint64,qint64); //跟新进度条

    void on_refresh_clicked();
    void on_alarmSend_clicked();
    void on_pcSend_clicked();
    void on_piGet_clicked();
    void showFolder();
    void showFolderBad();
    void on_weatherStat_clicked();
    void on_visitStat_clicked();
    void on_weatherStat2_clicked();
    void on_weatherStat3_clicked();
};
#endif // WIDGET_H
