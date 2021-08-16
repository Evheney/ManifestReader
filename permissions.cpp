#include "permissions.h"


QString Permissions::getPerm() const
{
    return permName;
}
BoardArray::BoardArray()
    : num(0), name(""), x(0), y(0), rotate(0), grName(0)
{
}

BoardArray::BoardArray(int n, QString nm, double xx, double yy, double rot, int grN)
    : num(n), name(nm), x(xx), y(yy), rotate(rot), grName(grN)
{
