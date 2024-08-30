/***************************************************************************
  qgsabstractrenderview.h
  --------------------------------------
  Date                 : June 2024
  Copyright            : (C) 2024 by Benoit De Mezzo
  Email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSABSTRACTRENDERVIEW_H
#define QGSABSTRACTRENDERVIEW_H

#include "qgis_3d.h"

#include <QObject>
#include <Qt3DRender/QRenderTargetOutput>

#define SIP_NO_FILE

class QColor;
class QRect;
class QSurface;

namespace Qt3DCore
{
  class QEntity;
}

namespace Qt3DRender
{
  class QRenderSettings;
  class QCamera;
  class QFrameGraphNode;
  class QLayer;
  class QViewport;
  class QTexture2D;
  class QSubtreeEnabler;
  class QRenderTargetSelector;
}

class QgsFrameGraph;

/**
 * \ingroup 3d
 * \brief Base class for 3D render view
 *
 * Will be used with QgsShadowRenderingFrameGraph::registerRenderView
 *
 * \note Not available in Python bindings
 * \since QGIS 3.40
 */
class _3D_EXPORT QgsAbstractRenderView : public QObject
{
    Q_OBJECT
  public:

    /**
     * Constructor for QgsAbstractRenderView with the specified \a parent object.
     */
    QgsAbstractRenderView( QObject *parent = nullptr, const QString &viewName = "unnamed_view" );

    //! set output to screen (ie. nullptr) or to a render target output
    virtual void setTargetOutputs( const QList<Qt3DRender::QRenderTargetOutput *> &targetOutputList );

    //! Returns list of all target outputs
    virtual QList<Qt3DRender::QRenderTargetOutput *> targetOutputs() const { return mTargetOutputs; }

    //! Updates map sizes for all target outputs
    virtual void updateTargetOutputSize( int width, int height );

    //! Returns the 2D texture attached at the \a attachment point, if any
    virtual Qt3DRender::QTexture2D *outputTexture( Qt3DRender::QRenderTargetOutput::AttachmentPoint attachment );

    //! Returns the layer to be used by entities to be included in this renderview
    virtual Qt3DRender::QLayer *layerToFilter();

    //! Returns the viewport associated to this renderview
    virtual Qt3DRender::QViewport *viewport();

    //! Returns the top node of this renderview branch. Will be used to register the renderview.
    virtual Qt3DRender::QFrameGraphNode *topGraphNode();

    //! Enable or disable via \a enable the renderview sub tree
    virtual void enableSubTree( bool enable );

    //! Returns true if renderview is enabled
    virtual bool isSubTreeEnabled();

  protected:
    std::pair<Qt3DRender::QFrameGraphNode *, Qt3DRender::QSubtreeEnabler *> createSubtreeEnabler( Qt3DRender::QFrameGraphNode *parent = nullptr );

    //! Handles target outputs changes
    virtual void onTargetOutputUpdate();

    //! Stores target outputs
    QList<Qt3DRender::QRenderTargetOutput *> mTargetOutputs;

    QString mViewName;
    Qt3DRender::QFrameGraphNode *mRoot = nullptr;
    Qt3DRender::QSubtreeEnabler *mRendererEnabler = nullptr;
    Qt3DRender::QLayer *mLayer = nullptr;
    Qt3DRender::QRenderTargetSelector *mRenderTargetSelector = nullptr;
};

#endif // QGSABSTRACTRENDERVIEW_H
