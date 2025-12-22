#ifndef QGS3DCREATECUBEDIALOG_H
#define QGS3DCREATECUBEDIALOG_H

#include "ui_createprimitivedialog.h"

#include "qgspoint.h"

#include <QDialog>

/**
 * Base class to primitive creation dialog
 * \since QGIS 3.44
 */
class Qgs3DCreatePrimitiveDialog : public QDialog, protected Ui::CreatePrimitiveDialog
{
    Q_OBJECT

  public:
    /**
     * Default constructor
     * \param type primitive name
     * \param f window flag
     */
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

    /**
     * Set 3 translation coordinates
     * \param point new translation
     */
    void setTranslation( const QgsPoint &point );

    /**
     * Set rotation values in degrees
     * \param rx x axis rotation
     * \param ry y axis rotation
     * \param rz z axis rotation
     */
    void setRotation( double rx, double ry, double rz );

    /**
     * Set 3 scale values
     * \param point new scales
     */
    void setScale( const QgsPoint &point );

    /// \return X translation value
    double transX() const { return spinTX->value(); }
    /// \return Y translation value
    double transY() const { return spinTY->value(); }
    /// \return Z translation value
    double transZ() const { return spinTZ->value(); }

    /// \return X axis rotation value
    double rotX() const { return spinRX->value(); }

    /// \return Y axis rotation value
    double rotY() const { return spinRY->value(); }

    /// \return Z axis rotation value
    double rotZ() const { return spinRZ->value(); }

    /// \return X axis scale value
    double scaleX() const { return spinSX->value(); }
    /// \return X axis scale value
    double scaleY() const { return spinSY->value(); }
    /// \return X axis scale value
    double scaleZ() const { return spinSZ->value(); }

    /// Focus on create button
    void focusCreateButton();
    /// Release focus on create button
    void unfocusCreateButton();

    /// \return the total number of parameter
    virtual int paramNumber() const = 0;

    /// \return the number of parameter the user will have to set
    virtual int creationParamNumber() const = 0;

    /**
     * \param idx parameter number
     * \return move constraint for the specified parameter
     */
    virtual ConstrainedAxis constrainedAxisForParam( int idx ) = 0;

    /**
     * Set new \a value for specified parameter
     * \param idx parameter number
     * \param value new value
     */
    virtual void setParam( int idx, double value );

    /**
     * \param idx parameter number
     * \return value for specified parameter
     */
    virtual double getParam( int idx ) const;

    /**
     * Bring focus on specific parameter
     * \param idx parameter number
     */
    virtual void focusOnParam( int idx );

    /// Reset dialog values
    virtual void resetData();

  signals:
    /// Emitted anytime a value is changed
    void valueChanged();
    /// Emitted when create button is clicked
    void createClicked( bool );

  protected:
    /**
     * \param idx parameter number
     * \return value for specified parameter
     */
    virtual QAbstractSpinBox *getSpinBox( int idx ) = 0;

  private:
    /// Primitive type name
    QString mType;
};

#endif // QGS3DCREATECUBEDIALOG_H
