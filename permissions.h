#ifndef PERMISSIONS_H
#define PERMISSIONS_H

#include <QString>
#include <QStringList>
#include <QList>

class Permissions
{
public:
    Permissions()
        :permName(""),permCount();
{

}
    ~Permissions();




    QString Perms;
    QStringList PermArray;
    QString getPerm() const;
};

#endif // PERMISSIONS_H
class Component
{
public:
    Component();
    ~Component();

    void addPin(qreal x, qreal y, qreal dia);
    void addRect(qreal x, qreal y, qreal w, qreal h, qreal rot);
    void addPoly(qreal x, qreal y, const QVector<QPointF> &listPts);
    //void setBoardArray()

    void setNumber(int n);
    void setPoint(qreal x, qreal y);
    void setSize(qreal sx, qreal sy);
    void setRealName(const QString &value);
    void setPartName(const QString &value);
    void setSide(const QString &value);
    void setRotateAngle(double value);

    QPointF getPoint() const;
    QSizeF getSize() const;
    int getNumShapes() const;
    Shape* getShape(int index) const;
    double rotateAngle() const;

    QString getRealName() const;
    QString getPartName() const;
    QString getSide() const;

    BoardArray getBoardArray() const;
    void setBoardArray(const BoardArray &value);
