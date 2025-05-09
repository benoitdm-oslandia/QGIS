/***************************************************************************
  qgsgoochmaterialsettings.h
  --------------------------------------
  Date                 : July 2020
  Copyright            : (C) 2020 by Nyall Dawson
  Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSGOOCHMATERIALSETTINGS_H
#define QGSGOOCHMATERIALSETTINGS_H

#include "qgis_3d.h"
#include "qgsabstractmaterialsettings.h"
#include "qgsmaterial.h"

#include <QColor>

class QDomElement;

/**
 * \ingroup qgis_3d
 * \brief Basic shading material used for rendering based on the Phong shading model
 * with three color components: ambient, diffuse and specular.
 *
 * \warning This is not considered stable API, and may change in future QGIS releases. It is
 * exposed to the Python bindings as a tech preview only.
 *
 * \since QGIS 3.16
 */
class _3D_EXPORT QgsGoochMaterialSettings : public QgsAbstractMaterialSettings
{
  public:
    QgsGoochMaterialSettings() = default;

    QString type() const override;

    /**
     * Returns a new instance of QgsGoochMaterialSettings.
     */
    static QgsAbstractMaterialSettings *create() SIP_FACTORY;

    /**
     * Returns TRUE if the specified \a technique is supported by the Gooch material.
     */
    static bool supportsTechnique( QgsMaterialSettingsRenderingTechnique technique );

    QgsGoochMaterialSettings *clone() const override SIP_FACTORY;
    bool equals( const QgsAbstractMaterialSettings *other ) const override;

    //! Returns warm color component
    QColor warm() const { return mWarm; }

    //! Returns cool color component
    QColor cool() const { return mCool; }

    //! Returns diffuse color component
    QColor diffuse() const { return mDiffuse; }
    //! Returns specular color component
    QColor specular() const { return mSpecular; }
    //! Returns shininess of the surface
    double shininess() const { return mShininess; }

    //! Returns the alpha value
    double alpha() const { return mAlpha; }

    //! Returns the beta value
    double beta() const { return mBeta; }

    //! Sets warm color component
    void setWarm( const QColor &warm ) { mWarm = warm; }

    //! Sets cool color component
    void setCool( const QColor &cool ) { mCool = cool; }

    //! Sets diffuse color component
    void setDiffuse( const QColor &diffuse ) { mDiffuse = diffuse; }
    //! Sets specular color component
    void setSpecular( const QColor &specular ) { mSpecular = specular; }
    //! Sets shininess of the surface
    void setShininess( double shininess ) { mShininess = shininess; }

    //! Sets alpha value
    void setAlpha( double alpha ) { mAlpha = alpha; }

    //! Sets beta value
    void setBeta( double beta ) { mBeta = beta; }

    void readXml( const QDomElement &elem, const QgsReadWriteContext &context ) override;
    void writeXml( QDomElement &elem, const QgsReadWriteContext &context ) const override;
    QMap<QString, QString> toExportParameters() const override;

#ifndef SIP_RUN
    QgsMaterial *toMaterial( QgsMaterialSettingsRenderingTechnique technique, const QgsMaterialContext &context ) const override;
    void addParametersToEffect( Qt3DRender::QEffect *effect, const QgsMaterialContext &materialContext ) const override;

    QByteArray dataDefinedVertexColorsAsByte( const QgsExpressionContext &expressionContext ) const override;
    int dataDefinedByteStride() const override;
#if QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
    void applyDataDefinedToGeometry( Qt3DRender::QGeometry *geometry, int vertexCount, const QByteArray &data ) const override;
#else
    void applyDataDefinedToGeometry( Qt3DCore::QGeometry *geometry, int vertexCount, const QByteArray &data ) const override;
#endif

#endif

    // TODO c++20 - replace with = default
    bool operator==( const QgsGoochMaterialSettings &other ) const
    {
      return mDiffuse == other.mDiffuse && mSpecular == other.mSpecular && mWarm == other.mWarm && mCool == other.mCool && qgsDoubleNear( mShininess, other.mShininess ) && qgsDoubleNear( mAlpha, other.mAlpha ) && qgsDoubleNear( mBeta, other.mBeta ) && dataDefinedProperties() == other.dataDefinedProperties();
    }

  private:
    QColor mDiffuse { QColor::fromRgbF( 0.7f, 0.7f, 0.7f, 1.0f ) };
    QColor mSpecular { QColor::fromRgbF( 1.0f, 1.0f, 1.0f, 1.0f ) };
    QColor mWarm { QColor( 107, 0, 107 ) };
    QColor mCool { QColor( 255, 130, 0 ) };
    double mShininess = 100.0;
    double mAlpha = 0.25;
    double mBeta = 0.5;

    //! Constructs a material from shader files
    QgsMaterial *buildMaterial( const QgsMaterialContext &context ) const;
};


#endif // QGSGOOCHMATERIALSETTINGS_H
