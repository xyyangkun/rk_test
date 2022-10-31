#ifndef MUITEMDELEGATE_H
#define MUITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QMetaType>

struct MuItemData{
    //图标
    QString iconPath;
    //文件名
    QString fileName;
    //帧率
    QString rate;
    //时长
    QString duration;
    //待定
    QString other;
};

Q_DECLARE_METATYPE(MuItemData)

class MuItemDelegate : public QStyledItemDelegate
{
public:
    MuItemDelegate(QObject *parent = nullptr);

    // painting
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const Q_DECL_OVERRIDE;
};


#endif // MUITEMDELEGATE_H
