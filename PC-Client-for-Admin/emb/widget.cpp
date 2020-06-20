#include "widget.h"
#include "ui_widget.h"
#include <QtNetwork>
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QTableView>
#include <QStandardItemModel>
#include <QDesktopServices>
#include "global.cpp"
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLegend>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
QT_CHARTS_USE_NAMESPACE

QList<Weather> weatherData;
QList<Visit> visitData;

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    ui->pcMsg->setAcceptRichText(false);
    updateUI=false;
}

Widget::~Widget()
{
    delete ui;
}

void Widget::startRequest(QUrl url)
{
    reply = manager->get(QNetworkRequest(url));//get()函数发送网络请求

    connect(reply,SIGNAL(readyRead()),this,SLOT(httpReadyRead()));  //有可用数据时
    connect(reply,SIGNAL(downloadProgress(qint64,qint64)),this,SLOT(updateDataReadProgress(qint64,qint64))); //更新进度条
    connect(reply,SIGNAL(finished()),this,SLOT(httpFinished()));    //下载完成后
}

QString Widget::downloadfolder(QString dir)
{
    return "";
}

void Widget::downControlA()
{
    switch(downCount)
    {
        case 1:
        {
        url="http://serveraddress/faceOut.txt";
        QFileInfo info(url.path());
        QString fileName(info.fileName());
        file = new QFile(fileName);
        if(!file->open(QIODevice::WriteOnly))
        {   //如果打开文件失败，则删除file，并使file指针为0，然后返回
            qDebug()<<"file open error";
            delete file;
            file = 0;
            return;
        }
        startRequest(url);
        downCount=2;
        break;
        }

    case 2:
    {
        url="http://serveraddress/weatherOut.txt";
        QFileInfo info(url.path());
        QString fileName(info.fileName());
        file = new QFile(fileName);
        if(!file->open(QIODevice::WriteOnly))
        {   //如果打开文件失败，则删除file，并使file指针为0，然后返回
            qDebug()<<"file open error";
            delete file;
            file = 0;
            return;
        }
        startRequest(url);
        downCount=3;
        break;
    }

    case 3:
    {
        url="http://serveraddress/good.txt";
        QFileInfo info(url.path());
        QString fileName(info.fileName());
        file = new QFile(fileName);
        if(!file->open(QIODevice::WriteOnly))
        {   //如果打开文件失败，则删除file，并使file指针为0，然后返回
            qDebug()<<"file open error";
            delete file;
            file = 0;
            return;
        }
        startRequest(url);
        downCount=4;
        break;
    }

    case 4:
    {
        url="http://serveraddress/bad.txt";
        QFileInfo info(url.path());
        QString fileName(info.fileName());
        file = new QFile(fileName);
        if(!file->open(QIODevice::WriteOnly))
        {   //如果打开文件失败，则删除file，并使file指针为0，然后返回
            qDebug()<<"file open error";
            delete file;
            file = 0;
            return;
        }
        startRequest(url);
        downCount=5;
        break;
    }

    case 5:
    {
        if(goodPtr==-1)
        {
            prepareList();
            goodPtr=0;
            badPtr=0;
        }
            url="http://serveraddress/good/"+good[goodPtr];
            QFileInfo info(url.path());
            QString fileName(info.fileName());
            file = new QFile("good/"+fileName);
            qDebug()<<url;
            if(!file->open(QIODevice::WriteOnly))
            {   //如果打开文件失败，则删除file，并使file指针为0，然后返回
                qDebug()<<"file open error";
                delete file;
                file = 0;
                return;
            }
            goodPtr++;
            if(goodPtr==good.size()-1)
                downCount=6;
            startRequest(url);
        break;
     }

    case 6:
    {
            url="http://serveraddress/bad/"+bad[badPtr];
            qDebug()<<badPtr;
            QFileInfo info(url.path());
            QString fileName(info.fileName());
            file = new QFile("bad/"+fileName);
            qDebug()<<url;
            if(!file->open(QIODevice::WriteOnly))
            {   //如果打开文件失败，则删除file，并使file指针为0，然后返回
                qDebug()<<"file open error";
                delete file;
                file = 0;
                return;
            }
            badPtr++;
            if(badPtr==bad.size()-1)
                downCount=7;
            startRequest(url);
        break;
     }

    case 7:analysisData();
        qDebug()<<"yes!";
        break;
    }
}

void Widget::prepareList()
{
    QFile goodtxt("good.txt");
    goodtxt.open(QIODevice::ReadOnly | QIODevice::Text);
    QString tmpData=goodtxt.readAll();
    goodtxt.close();
    good=tmpData.split('\n');

    QFile badtxt("bad.txt");
    badtxt.open(QIODevice::ReadOnly | QIODevice::Text);
    QString tmpData2=badtxt.readAll();
    badtxt.close();
    bad=tmpData2.split('\n');
}

void Widget::analysisData()
{
    char dump[1000];
    QFile file("weatherOut.txt");
    file.open(QIODevice::ReadOnly|QIODevice::Text);

    Weather tmpw;
    while(!file.atEnd())
    {
        file.readLine(dump,1000);
        QStringList dumpl=QString(dump).split(' ');
        tmpw.date=dumpl[0]+"  "+dumpl[1];
        tmpw.temp=dumpl[2];
        tmpw.pressure=dumpl[3];
        tmpw.humid=dumpl[4].remove('\n');
        weatherData.push_back(tmpw);
    }
    file.close();

    QFile file2("faceOut.txt");
    file2.open(QIODevice::ReadOnly|QIODevice::Text);
    Visit tmpv;
    while(!file2.atEnd())
    {
        file2.readLine(dump,1000);
        QStringList dumpl=QString(dump).split(' ');
        tmpv.date=dumpl[0]+"  "+dumpl[1];
        tmpv.id=dumpl[2];
        tmpv.name=dumpl[3].remove('\n');
        visitData.push_back(tmpv);
    }
    file2.close();

    {
    QStandardItemModel* model = new QStandardItemModel(ui->weatherTable);
    model->setColumnCount(4);
    model->setHeaderData(0,Qt::Horizontal,"                 采集时间                ");
    model->setHeaderData(1,Qt::Horizontal,"                 温度（℃）              ");
    model->setHeaderData(2,Qt::Horizontal,"                 气压（hPa）              ");
    model->setHeaderData(3,Qt::Horizontal,"                 湿度（%）               ");
    for(int i=0;i<weatherData.size();i++)
    {
        model->setItem(i,0,new QStandardItem(weatherData[i].date));
        model->setItem(i,1,new QStandardItem(weatherData[i].temp+" ℃"));
        model->setItem(i,2,new QStandardItem(weatherData[i].pressure+" hPa"));
        model->setItem(i,3,new QStandardItem(weatherData[i].humid+" %"));
    }
    ui->weatherTable->setModel(model);
    ui->weatherTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->weatherTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    }

    {
    QStandardItemModel* model = new QStandardItemModel(ui->visitTable);
    model->setColumnCount(4);
    model->setHeaderData(0,Qt::Horizontal,"             记录时间                 ");
    model->setHeaderData(1,Qt::Horizontal,"             来访者编号                ");
    model->setHeaderData(2,Qt::Horizontal,"             来访者名称                ");
    model->setHeaderData(3,Qt::Horizontal,"                 查看采集照片                      ");
    ui->visitTable->setModel(model);
    for(int i=0;i<visitData.size();i++)
    {
        model->setItem(i,0,new QStandardItem(visitData[i].date));
        model->setItem(i,1,new QStandardItem((visitData[i].id=="-1"?"未知人员":visitData[i].id)));
        model->setItem(i,2,new QStandardItem((visitData[i].name=="unauthorized"?"未知人员":visitData[i].name)));
        QPushButton *button=new QPushButton("点击查看采集照片");
        if(visitData[i].id=="-1")
            connect(button,SIGNAL(clicked()),this,SLOT(showFolderBad()));
        else connect(button,SIGNAL(clicked()),this,SLOT(showFolder()));
        ui->visitTable->setIndexWidget(model->index(i,3),button);
    }
    ui->visitTable->setModel(model);
    ui->visitTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->visitTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    }
}

void Widget::httpFinished()
{
    qDebug()<<url;
    file->flush();
    file->close();
    reply->deleteLater();
    reply = 0;
    delete file;
    file = 0;
    downControlA();
    if(updateUI)
    {
        updateUI=false;
        QFile msg("msg.txt");
        msg.open(QIODevice::ReadOnly|QIODevice::Text);
        auto res=msg.readAll();
        msg.close();
        ui->piMsg->setText(res);
    }
}

void Widget::httpReadyRead()
{
    if(file)file->write(reply->readAll());  //如果文件存在，则写入文件
}

void Widget::updateDataReadProgress(qint64, qint64)
{

}

void Widget::on_refresh_clicked()
{
    manager = new QNetworkAccessManager(this);
    downCount=1;
    goodPtr=-1;
    badPtr=-1;
    good.clear();
    bad.clear();
    downControlA();
}

void Widget::on_alarmSend_clicked()
{
    QFile file("D:\\remote.txt");
    QString out;
    if(ui->alarmOn->isChecked())
        out="1 ";
    else out="0 ";
    out+=ui->pcMsg->toPlainText();
    QTextStream out2(&file);
    qDebug()<<file.open(QIODevice::WriteOnly|QIODevice::Text);
    out2.setCodec("UTF-8");
    out2<<out;
    file.close();
}

void Widget::on_pcSend_clicked()
{
    on_alarmSend_clicked();
}

void Widget::on_piGet_clicked()
{
    url="http://serveraddress/msg.txt";
    QFileInfo info(url.path());
    QString fileName(info.fileName());
    file = new QFile(fileName);
    if(!file->open(QIODevice::WriteOnly))
    {   //如果打开文件失败，则删除file，并使file指针为0，然后返回
        qDebug()<<"file open error";
        delete file;
        file = 0;
        return;
    }
    updateUI=true;
    downCount=7;
    startRequest(url);

}

void Widget::showFolder()
{
    QDesktopServices::openUrl(QUrl("good"));

}

void Widget::showFolderBad()
{
    QDesktopServices::openUrl(QUrl("bad"));
}

void Widget::on_weatherStat_clicked()
{
    QLineSeries *series = new QLineSeries();
    for(int i=0;i<weatherData.size();i++)
        series->append(i+1,weatherData[i].temp.toDouble());
    QChart *chart = new QChart();
    chart->legend()->hide();
        chart->addSeries(series);
        chart->createDefaultAxes();
        chart->setTitle("用鼠标框选某一区域可以放大到该区域，右键单击可以缩小显示区域。");
        chart->setFont(QFont("微软雅黑",20,10));
        QChartView *chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->setRubberBand(QChartView::VerticalRubberBand);
        chartView->setRubberBand(QChartView::HorizontalRubberBand);
        chartView->setFont(QFont("微软雅黑",20,10));
        chartView->setStyleSheet("font: 75 12px \"微软雅黑\";");
        QMainWindow *window=new QMainWindow();
        window->setCentralWidget(chartView);
        window->setFont(QFont("微软雅黑",20,10));
        window->resize(1000, 600);
        window->show();
}

void Widget::on_visitStat_clicked()
{
    QBarSet *set0 = new QBarSet("授权访问次数");
    QBarSet *set1 = new QBarSet("非法访问次数");

     int goodnum=0,badnum=0;
     for(int i=0;i<visitData.size();i++)
     {
         if(visitData[i].id=="-1")
             badnum++;
         else goodnum++;
     }

     *set0<<goodnum;
     *set1<<badnum;

     QBarSeries *series = new QBarSeries();
     series->append(set0);
     series->append(set1);

     QChart *chart = new QChart();
     chart->addSeries(series);

     chart->setTitle("授权和非法访问次数统计");
     chart->setAnimationOptions(QChart::SeriesAnimations);

     QValueAxis *axisY = new QValueAxis();
     chart->addAxis(axisY, Qt::AlignLeft);
     series->attachAxis(axisY);

     chart->legend()->setVisible(true);
     chart->legend()->setAlignment(Qt::AlignBottom);

     QChartView *chartView = new QChartView(chart);
     chartView->setRenderHint(QPainter::Antialiasing);

     QMainWindow *window=new QMainWindow();
     window->setCentralWidget(chartView);
     window->setFont(QFont("微软雅黑",20,10));
     window->resize(600, 500);
     window->show();
}

void Widget::on_weatherStat2_clicked()
{
    QLineSeries *series = new QLineSeries();
    for(int i=0;i<weatherData.size();i++)
        series->append(i+1,weatherData[i].humid.toDouble());
    QChart *chart = new QChart();
    chart->legend()->hide();
        chart->addSeries(series);
        chart->createDefaultAxes();
        chart->setTitle("用鼠标框选某一区域可以放大到该区域，右键单击可以缩小显示区域。");
        chart->setFont(QFont("微软雅黑",20,10));
        QChartView *chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->setRubberBand(QChartView::VerticalRubberBand);
        chartView->setRubberBand(QChartView::HorizontalRubberBand);
        chartView->setFont(QFont("微软雅黑",20,10));
        chartView->setStyleSheet("font: 75 12px \"微软雅黑\";");
        QMainWindow *window=new QMainWindow();
        window->setCentralWidget(chartView);
        window->setFont(QFont("微软雅黑",20,10));
        window->resize(1000, 600);
        window->show();
}

void Widget::on_weatherStat3_clicked()
{
    QLineSeries *series = new QLineSeries();
    for(int i=0;i<weatherData.size();i++)
        series->append(i+1,weatherData[i].pressure.toDouble());
    QChart *chart = new QChart();
    chart->legend()->hide();
        chart->addSeries(series);
        chart->createDefaultAxes();
        chart->setTitle("用鼠标框选某一区域可以放大到该区域，右键单击可以缩小显示区域。");
        chart->setFont(QFont("微软雅黑",20,10));
        QChartView *chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->setRubberBand(QChartView::VerticalRubberBand);
        chartView->setRubberBand(QChartView::HorizontalRubberBand);
        chartView->setFont(QFont("微软雅黑",20,10));

        chartView->setStyleSheet("font: 75 12px \"微软雅黑\";");
        QMainWindow *window=new QMainWindow();
        window->setCentralWidget(chartView);
        window->setFont(QFont("微软雅黑",20,10));
        window->resize(1000, 600);
        window->show();
}
