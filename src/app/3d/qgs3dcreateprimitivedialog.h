#ifndef QGS3DCREATECUBEDIALOG_H
#define QGS3DCREATECUBEDIALOG_H

#include "ui_createprimitivedialog.h"

#include "qgspoint.h"

#include <QDialog>

class Qgs3DCreatePrimitiveDialog : public QDialog, protected Ui::CreatePrimitiveDialog
{
    Q_OBJECT

  public:
    Qgs3DCreatePrimitiveDialog( const QString &type, Qt::WindowFlags f = ( Qt::WindowFlags() | Qt::Tool ) );

    /**
     * Mouse move constraint
     */
    enum ConstrainedAxis
    {
      NONE, //!< No constraint
      X,    //!< Only on X axis
      Y,    //!< Only on Y axis
      Z,    //!< Only on Z axis
      XY,   //!< Only on XY plane
      XZ,   //!< Only on XZ plane
      YZ    //!< Only on YZ plane
    };

    void setTranslation( const QgsPoint &point );
    void setRotation( double rx, double ry, double rz );
    void setScale( const QgsPoint &point );

    double transX() const { return spinTX->value(); }
    double transY() const { return spinTY->value(); }
    double transZ() const { return spinTZ->value(); }

    double rotX() const { return spinRX->value(); }
    double rotY() const { return spinRY->value(); }
    double rotZ() const { return spinRZ->value(); }

    double scaleX() const { return spinSX->value(); }
    double scaleY() const { return spinSY->value(); }
    double scaleZ() const { return spinSZ->value(); }

    /// Focus on create button
    void focusCreateButton();
    /// Release focus on create button
    void unfocusCreateButton();

    /// \return the number of paramter the user will have to set
    virtual int paramNumber() const = 0;

    /**
     * \param idx parameter number
     * \return move constraint for the specified parameter
     */
    virtual ConstrainedAxis constrainedAxisForParam( int idx ) = 0;

    virtual void setParam( int idx, double value ) = 0;
    virtual double getParam( int idx ) const = 0;
    virtual void resetData();

  signals:
    void valueChanged();
    /// Emited when create button is clicked
    void createClicked( bool );

  private:
    QString mType;
};

#endif // QGS3DCREATECUBEDIALOG_H
