#ifndef QGS3DCREATEPRIMITIVESPHEREDIALOG_H
#define QGS3DCREATEPRIMITIVESPHEREDIALOG_H

#include "qgs3dcreateprimitivedialog.h"

class Qgs3DCreatePrimitiveSphereDialog : public Qgs3DCreatePrimitiveDialog
{
    Q_OBJECT
  public:
    Qgs3DCreatePrimitiveSphereDialog( Qt::WindowFlags f = ( Qt::WindowFlags() | Qt::Tool ) );

    void resetData() override;

    int paramNumber() const override { return 1; };
    ConstrainedAxis constrainedAxisForParam( int idx ) override;
    void setParam( int idx, double value ) override;
    double getParam( int idx ) const override;

    void setRadius( double size );
    void setRings( int size );
    void setSlices( int size );

  private:
    QDoubleSpinBox *mSpinRadius;
    QSpinBox *mSpinRings;
    QSpinBox *mSpinSlices;
};

#endif // QGS3DCREATEPRIMITIVESPHEREDIALOG_H
