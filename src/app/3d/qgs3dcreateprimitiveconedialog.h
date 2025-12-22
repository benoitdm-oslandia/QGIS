#ifndef QGS3DCREATEPRIMITIVECONEDIALOG_H
#define QGS3DCREATEPRIMITIVECONEDIALOG_H

#include "qgs3dcreateprimitivedialog.h"

class Qgs3DCreatePrimitiveConeDialog : public Qgs3DCreatePrimitiveDialog
{
    Q_OBJECT
  public:
    Qgs3DCreatePrimitiveConeDialog( Qt::WindowFlags f = ( Qt::WindowFlags() | Qt::Tool ) );

    void resetData() override;
    void setBottomRadius( double size );
    void setLength( double size );
    void setTopRadius( double size );
    void setRings( int size );
    void setSlices( int size );

    int paramNumber() const override { return 3; };
    ConstrainedAxis constrainedAxisForParam( int idx ) override;
    void setParam( int idx, double value ) override;
    double getParam( int idx ) const override;

  private:
    QDoubleSpinBox *mSpinBottomRadius;
    QDoubleSpinBox *mSpinLength;
    QDoubleSpinBox *mSpinTopRadius;
    QSpinBox *mSpinRings;
    QSpinBox *mSpinSlices;
};

#endif // QGS3DCREATEPRIMITIVECONEDIALOG_H
