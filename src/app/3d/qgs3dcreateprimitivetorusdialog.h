#ifndef QGS3DCREATEPRIMITIVETORUSDIALOG_H
#define QGS3DCREATEPRIMITIVETORUSDIALOG_H

#include "qgs3dcreateprimitivedialog.h"

class Qgs3DCreatePrimitiveTorusDialog : public Qgs3DCreatePrimitiveDialog
{
    Q_OBJECT
  public:
    Qgs3DCreatePrimitiveTorusDialog( Qt::WindowFlags f = ( Qt::WindowFlags() | Qt::Tool ) );

    void resetData() override;
    void setRadius( double size );
    void setMinorRadius( double size );
    void setRings( int size );
    void setSlices( int size );

    int paramNumber() const override { return 2; };
    ConstrainedAxis constrainedAxisForParam( int idx ) override;
    void setParam( int idx, double value ) override;
    double getParam( int idx ) const override;

  private:
    QDoubleSpinBox *mSpinRadius;
    QDoubleSpinBox *mSpinMinorRadius;
    QSpinBox *mSpinRings;
    QSpinBox *mSpinSlices;
};

#endif // QGS3DCREATEPRIMITIVETORUSDIALOG_H
