#ifndef QGS3DCREATEPRIMITIVESPHEREDIALOG_H
#define QGS3DCREATEPRIMITIVESPHEREDIALOG_H

#include "qgs3dcreateprimitivedialog.h"

class Qgs3DCreatePrimitiveSphereDialog : public Qgs3DCreatePrimitiveDialog
{
    Q_OBJECT
  public:
    Qgs3DCreatePrimitiveSphereDialog( Qt::WindowFlags f = ( Qt::WindowFlags() | Qt::Tool ) );

    void resetData() override;

    int paramNumber() const override { return 2; };
    int creationParamNumber() const override { return 1; };
    ConstrainedAxis constrainedAxisForParam( int idx ) override;

    void setRadius( double size );
    void setSubdivisions( unsigned int subdivisions );

  protected:
    QAbstractSpinBox *getSpinBox( int idx ) override;

  private:
    QDoubleSpinBox *mSpinRadius;
    QSpinBox *mSpinSubdivisions;
};

#endif // QGS3DCREATEPRIMITIVESPHEREDIALOG_H
