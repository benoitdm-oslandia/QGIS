#ifndef QGS3DCREATECUBEDIALOG_H
#define QGS3DCREATECUBEDIALOG_H

#include <QDialog>
#include "ui_createprimitivedialog.h"
#include "qgspoint.h"

class Qgs3DCreatePrimitiveDialog : public QDialog, protected Ui::CreatePrimitiveDialog
{
    Q_OBJECT

  public:
    Qgs3DCreatePrimitiveDialog( const QString &type, Qt::WindowFlags f = ( Qt::WindowFlags() | Qt::Tool ) );

    void setTranslation( const QgsPoint &point );
    void setRotation( double rx, double ry, double rz );
    void setScale( const QgsPoint &point );

    double rotX() const { return spinRX->value(); }
    double rotY() const { return spinRY->value(); }
    double rotZ() const { return spinRZ->value(); }

    double scaleX() const { return spinSX->value(); }
    double scaleY() const { return spinSY->value(); }
    double scaleZ() const { return spinSZ->value(); }

    virtual double defaultSize() = 0;
    virtual void resetData();

  signals:
    void valueChanged();

  private:
    QString mType;
};

#endif // QGS3DCREATECUBEDIALOG_H
