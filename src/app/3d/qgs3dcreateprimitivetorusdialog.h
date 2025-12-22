#ifndef QGS3DCREATEPRIMITIVETORUSDIALOG_H
#define QGS3DCREATEPRIMITIVETORUSDIALOG_H

#include "qgs3dcreateprimitivedialog.h"

class Qgs3DCreatePrimitiveTorusDialog : public Qgs3DCreatePrimitiveDialog
{
    Q_OBJECT
  public:
    Qgs3DCreatePrimitiveTorusDialog( Qt::WindowFlags f = ( Qt::WindowFlags() | Qt::Tool ) );

    void resetData() override;
    void setMainRadius( double size );
    void setTubeRadius( double size );
    void setMainRadial( int size );
    void setTubeRadial( int size );

    int paramNumber() const override { return 2; };
    ConstrainedAxis constrainedAxisForParam( int idx ) override;
    void setParam( int idx, double value ) override;
    double getParam( int idx ) const override;

  private:
    QDoubleSpinBox *mSpinMainRadius;
    QDoubleSpinBox *mSpinTubeRadius;
    QSpinBox *mSpinMainRadial;
    QSpinBox *mSpinTubeRadial;
};

#endif // QGS3DCREATEPRIMITIVETORUSDIALOG_H
