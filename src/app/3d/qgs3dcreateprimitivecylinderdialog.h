#ifndef QGS3DCREATEPRIMITIVECYLINDERDIALOG_H
#define QGS3DCREATEPRIMITIVECYLINDERDIALOG_H

#include "qgs3dcreateprimitivedialog.h"

class Qgs3DCreatePrimitiveCylinderDialog : public Qgs3DCreatePrimitiveDialog
{
    Q_OBJECT
  public:
    Qgs3DCreatePrimitiveCylinderDialog( Qt::WindowFlags f = ( Qt::WindowFlags() | Qt::Tool ) );

    void resetData() override;
    void setRadius( double size );
    void setLength( double size );
    void setRadial( int size );
    void setSlices( int size );

    int paramNumber() const override { return 3; };
    int creationParamNumber() const override { return 2; };
    ConstrainedAxis constrainedAxisForParam( int idx ) override;

  protected:
    QAbstractSpinBox *getSpinBox( int idx ) override;

  private:
    QDoubleSpinBox *mSpinRadius;
    QDoubleSpinBox *mSpinLength;
    QSpinBox *mSpinRadial;
    QSpinBox *mSpinSlices;
};

#endif // QGS3DCREATEPRIMITIVECYLINDERDIALOG_H
