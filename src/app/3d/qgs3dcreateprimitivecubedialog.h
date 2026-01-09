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

    int paramNumber() const override { return 4; };
    int creationParamNumber() const override { return 3; };
    ConstrainedAxis constrainedAxisForParam( int idx ) override;

  protected:
    QAbstractSpinBox *getSpinBox( int idx ) override;

  private:
    QDoubleSpinBox *mSpinSizeX;
    QDoubleSpinBox *mSpinSizeY;
    QDoubleSpinBox *mSpinSizeZ;
};

#endif // QGS3DCREATEPRIMITIVECUBEDIALOG_H
