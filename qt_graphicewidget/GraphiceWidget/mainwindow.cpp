#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QScrollBar>
#include <QScroller>
#include <QStandardItemModel>
#include "muitemdelegate.h"
#include <QTimer>
#include <QDebug>
#include <QDateTime>

QStandardItemModel *pModel;
MuItemDelegate *pItemDelegate;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->listView->setSpacing(0);
    ui->listView->installEventFilter(this);
    QScroller::grabGesture(ui->listView,QScroller::LeftMouseButtonGesture);

    ui->listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 一直关闭垂直滑动条
    ui->listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 一直关闭水平滑动条
    ui->listView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel); // 设置像素级滑动

    ui->listView->setFlow(QListView::LeftToRight); //设置从左到右排列
    ui->listView->setFlow(QListView::TopToBottom); //设置从上到下排列

    auto sc = QScroller::scroller(ui->listView);
    auto ps = sc->scrollerProperties();
    ps.setScrollMetric(QScrollerProperties::FrameRate,QScrollerProperties::Fps30);
    sc->setScrollerProperties(ps);
    pModel = new QStandardItemModel();
    pItemDelegate = new MuItemDelegate(this);

    ui->listView->setItemDelegate(pItemDelegate);
    ui->listView->setModel(pModel);

    for(int i = 0;i < 150;++i)
    {
        QStandardItem *pItem = new QStandardItem;
        MuItemData itemData;
        itemData.fileName = tr("这些材料箱子开出来现asdasdasd在库存1121ds");
        pItem->setData(QVariant::fromValue(itemData), Qt::UserRole+1);
        pModel->appendRow(pItem);
    }

    ui->listView->setStyleSheet("border-image:url(:/RC.png)");
}

MainWindow::~MainWindow()
{
    delete ui;
}
