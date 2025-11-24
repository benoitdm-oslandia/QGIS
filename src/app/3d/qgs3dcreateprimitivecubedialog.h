#ifndef QGS3DCREATEPRIMITIVECUBEDIALOG_H
#define QGS3DCREATEPRIMITIVECUBEDIALOG_H

#include "qgs3dcreateprimitivedialog.h"

class Qgs3DCreatePrimitiveCubeDialog : public Qgs3DCreatePrimitiveDialog
{
    Q_OBJECT
  public:
    Qgs3DCreatePrimitiveCubeDialog( Qt::WindowFlags f = ( Qt::WindowFlags() | Qt::Tool ) );

    void resetData() override;
    void setSizeX( double size );
    void setSizeY( double size );
    void setSizeZ( double size );

    int paramNumber() const override { return 3; };
    void setParam( int idx, double value ) override;
    double getParam( int idx ) const override;

  private:
    QDoubleSpinBox *mSpinSizeX;
    QDoubleSpinBox *mSpinSizeY;
    QDoubleSpinBox *mSpinSizeZ;
};

#endif // QGS3DCREATEPRIMITIVECUBEDIALOG_H
