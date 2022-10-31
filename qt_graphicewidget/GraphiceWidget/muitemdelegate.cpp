#include <QPainter>
#include <QDebug>
#include <QFont>
#include <QLabel>
#include <QMovie>
#include <QTimer>
#include "muitemdelegate.h"

extern QMovie *movie;

MuItemDelegate::MuItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{

}

void MuItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    //使qss生效
    QStyledItemDelegate::paint(painter, option, index);

    if (index.isValid()) {
        painter->save();
        QVariant var = index.data(Qt::UserRole+1);
        MuItemData itemData = var.value<MuItemData>();

        // item 矩形区域
        QRectF rect;
        rect.setX(option.rect.x());
        rect.setY(option.rect.y());
        rect.setWidth(option.rect.width()-1);
        rect.setHeight(option.rect.height()-1);

        QPainterPath path;
        path.moveTo(rect.topRight());
        path.lineTo(rect.topLeft());
        path.quadTo(rect.topLeft(), rect.topLeft());
        path.lineTo(rect.bottomLeft());
        path.quadTo(rect.bottomLeft(), rect.bottomLeft());
        path.lineTo(rect.bottomRight());
        path.quadTo(rect.bottomRight(), rect.bottomRight());
        path.lineTo(rect.topRight());
        path.quadTo(rect.topRight(), rect.topRight());

        // 绘制数据
//        QRectF iconRect = QRect(rect.left()+40, rect.top()+26, 48, 48);
        QRectF fileNameRect = QRect(rect.left()+144, rect.top()+26, 550, 52);//48
//        QRectF rateRect = QRect(rect.left()+770, rect.top()+26, 210, 52);
//        QRectF durationRect = QRect(rect.left()+1000, rect.top()+26, 160, 52);

//        painter->drawImage(iconRect, QImage(itemData.iconPath));

        painter->setPen(QPen(Qt::black));
        QFont font(painter->font());
        font.setPixelSize(38);
        painter->setFont(font);
        auto fm = painter->fontMetrics();
        auto fileName = fm.elidedText(itemData.fileName.toLocal8Bit(),Qt::ElideMiddle,550,0);
        painter->drawText(fileNameRect, fileName);//
//        painter->drawText(rateRect, itemData.rate);
//        painter->drawText(durationRect, itemData.duration);

        painter->restore();
    }
}

QSize MuItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QSize(option.rect.width(), 100);
}

