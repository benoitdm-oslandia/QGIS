#ifndef QGS3DCREATECUBEDIALOG_H
#define QGS3DCREATECUBEDIALOG_H

#include <QDialog>
#include "ui_createprimitivedialog.h"
#include "qgspoint.h"

class Qgs3DCreateCubeDialog : public QDialog, private Ui::CreatePrimitiveDialog
{
    Q_OBJECT

  public:
    Qgs3DCreateCubeDialog( Qt::WindowFlags f = ( Qt::WindowFlags() | Qt::Tool ) );

    void setTranslation( const QgsPoint &point );
    void setRotation( double rx, double ry, double rz );
    void setSize( double size );

    void resetData();
};

#endif // QGS3DCREATECUBEDIALOG_H
