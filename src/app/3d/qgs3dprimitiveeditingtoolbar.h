/***************************************************************************
    qgs3dprimitiveeditingtoolbar.h
    -------------------
    begin                : November 2025
    copyright            : (C) 2025 Oslandia
    email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGS3DPRIMITIVEEDITINGTOOLBAR_H
#define QGS3DPRIMITIVEEDITINGTOOLBAR_H

#include "qgs3deditingtoolbar.h"
#include "qgs3dmaptoolcreateprimitive.h"

class QgsMapLayer;
class Qgs3DMapCanvasWidgetInterface;

/**
 * Allow creation of 3D primitive on polyhedral layers
 *
 * \since QGIS 3.44
 */
class Qgs3DPrimitiveEditingToolBar : public Qgs3DEditingToolBar
{
    Q_OBJECT

  public:
    /**
     * Default constructor
     * \param parent parent widget
     */
    Qgs3DPrimitiveEditingToolBar( Qgs3DMapCanvasWidgetInterface *parent );
    bool accept( QgsMapLayer *layer ) const override;
    void activate( QgsMapLayer *layer ) override;
    void deactivate() override;
    QList<QAction *> groupActions() const override;

    QgsMapLayer *activeLayer() const { return mActiveLayer; }

  private slots:
    void createCube();
    void createSphere();
    void createTorus();
    void createCylinder();
    void createCone();
    void createPrimitive( const QAction *action, Qgs3DMapToolCreatePrimitive::PrimitiveType type );

  private:
    QAction *mCreatePrimitiveAction = nullptr;
    QList<QAction *> mActions;
    QgsMapLayer *mActiveLayer = nullptr;

    Qgs3DMapToolCreatePrimitive *mCreatePrimitiveMapTool = nullptr;
};

#endif // QGS3DPRIMITIVEEDITINGTOOLBAR_H
