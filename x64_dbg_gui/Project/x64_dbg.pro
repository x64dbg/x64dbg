#-------------------------------------------------
#
# Project created by QtCreator 2013-05-20T13:22:23
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Removes all debug output when defined
#DEFINES += QT_NO_DEBUG_OUTPUT

#generate debug symbols in release mode
QMAKE_CFLAGS_RELEASE += -Zi
QMAKE_LFLAGS_RELEASE += /DEBUG

!contains(QMAKE_HOST.arch, x86_64) {
    TARGET = x32_gui
} else {
    TARGET = x64_gui
}

DEFINES += BUILD_LIB
TEMPLATE = lib
DEFINES += NOMINMAX
DEFINES += OGDF_DLL

INCLUDEPATH += $$PWD/Src/Bridge

LIBS += -luser32

!contains(QMAKE_HOST.arch, x86_64) {
    ## Windows x86 (32bit) specific build here
    LIBS += -L"$$PWD/Src/ThirdPartyLibs/BeaEngine/" -lBeaEngine
    LIBS += -L"$$PWD/Src/Bridge/" -lx32_bridge
    LIBS += -L"$$PWD/Src/ThirdPartyLibs/OGDF/" -logdf_x86
} else {
    ## Windows x64 (64bit) specific build here
    LIBS += -L"$$PWD/Src/ThirdPartyLibs/BeaEngine/" -lBeaEngine_64
    LIBS += -L"$$PWD/Src/Bridge/" -lx64_bridge
    LIBS += -L"$$PWD/Src/ThirdPartyLibs/OGDF/" -logdf_x64
}

SOURCES += \
    Src/main.cpp \
    Src/Gui/MainWindow.cpp \
    Src/Gui/CPUWidget.cpp \
    Src/Gui/CommandLineEdit.cpp \
    Src/BasicView/Disassembly.cpp \
    Src/BasicView/HexDump.cpp \
    Src/BasicView/AbstractTableView.cpp \
    Src/Disassembler/QBeaEngine.cpp \
    Src/Memory/MemoryPage.cpp \
    Src/Bridge/Bridge.cpp \
    Src/BasicView/StdTable.cpp \
    Src/Gui/MemoryMapView.cpp \
    Src/Gui/LogView.cpp \
    Src/Gui/GotoDialog.cpp \
    Src/Gui/StatusLabel.cpp \
    Src/Gui/WordEditDialog.cpp \
    Src/Gui/CPUDisassembly.cpp \
    Src/Gui/LineEditDialog.cpp \
    Src/Gui/BreakpointsView.cpp \
    Src/Utils/Breakpoints.cpp \
    Src/Gui/CPUInfoBox.cpp \
    Src/Gui/CPUDump.cpp \
    Src/Gui/ScriptView.cpp \
    Src/Gui/CPUStack.cpp \
    Src/Gui/SymbolView.cpp \
    Src/Gui/RegistersView.cpp \
    Src/BasicView/SearchListView.cpp \
    Src/BasicView/ReferenceView.cpp \
    Src/Gui/ThreadView.cpp \
    Src/Gui/SettingsDialog.cpp \
    Src/Gui/ExceptionRangeDialog.cpp \
    Src/Utils/RichTextPainter.cpp \
    Src/Gui/TabBar.cpp \
    Src/Gui/TabWidget.cpp \
    Src/Gui/CommandHelpView.cpp \
    Src/BasicView/HistoryLineEdit.cpp \
    Src/Utils/Configuration.cpp \
    Src/Gui/CPUSideBar.cpp \
    Src/Gui/AppearanceDialog.cpp \
    Src/Disassembler/BeaTokenizer.cpp \
    Src/Gui/CloseDialog.cpp \
    Src/Gui/HexEditDialog.cpp \
    Src/QHexEdit/ArrayCommand.cpp \
    Src/QHexEdit/QHexEdit.cpp \
    Src/QHexEdit/QHexEditPrivate.cpp \
    Src/QHexEdit/XByteArray.cpp \
    Src/Gui/PatchDialog.cpp \
    Src/Gui/PatchDialogGroupSelector.cpp \
    Src/Utils/UpdateChecker.cpp \
    Src/BasicView/SearchListViewTable.cpp \
    Src/Gui/CallStackView.cpp \
    Src/Gui/ShortcutsDialog.cpp \
    Src/BasicView/ShortcutEdit.cpp \
    Src/Gui/CalculatorDialog.cpp \
    Src/Gui/AttachDialog.cpp \
    Src/Gui/PageMemoryRights.cpp \
    Src/BasicView/FlowGraphScene.cpp \
    Src/BasicView/FlowGraphView.cpp \
    Src/BasicView/FlowGraphBlock.cpp \
    Src/BasicView/FlowGraphEdge.cpp \
    Src/Gui/FlowGraphWidget.cpp


HEADERS += \
    Src/main.h \
    Src/Gui/MainWindow.h \
    Src/Gui/CPUWidget.h \
    Src/Gui/CommandLineEdit.h \
    Src/BasicView/Disassembly.h \
    Src/BasicView/HexDump.h \
    Src/BasicView/AbstractTableView.h \
    Src/Disassembler/QBeaEngine.h \
    Src/Memory/MemoryPage.h \
    Src/Bridge/Bridge.h \
    Src/Global/NewTypes.h \
    Src/Exports.h \
    Src/Imports.h \
    Src/BasicView/StdTable.h \
    Src/Gui/MemoryMapView.h \
    Src/Gui/LogView.h \
    Src/Gui/GotoDialog.h \
    Src/Gui/RegistersView.h \
    Src/Gui/StatusLabel.h \
    Src/Gui/WordEditDialog.h \
    Src/Gui/CPUDisassembly.h \
    Src/Gui/LineEditDialog.h \
    Src/Gui/BreakpointsView.h \
    Src/Utils/Breakpoints.h \
    Src/Gui/CPUInfoBox.h \
    Src/Gui/CPUDump.h \
    Src/Gui/ScriptView.h \
    Src/Gui/CPUStack.h \
    Src/Gui/SymbolView.h \
    Src/BasicView/SearchListView.h \
    Src/BasicView/ReferenceView.h \
    Src/Gui/ThreadView.h \
    Src/Gui/SettingsDialog.h \
    Src/Gui/ExceptionRangeDialog.h \
    Src/Utils/RichTextPainter.h \
    Src/Gui/TabBar.h \
    Src/Gui/TabWidget.h \
    Src/Gui/CommandHelpView.h \
    Src/BasicView/HistoryLineEdit.h \
    Src/Utils/Configuration.h \
    Src/Gui/CPUSideBar.h \
    Src/Gui/AppearanceDialog.h \
    Src/Disassembler/BeaTokenizer.h \
    Src/Gui/CloseDialog.h \
    Src/Gui/HexEditDialog.h \
    Src/QHexEdit/ArrayCommand.h \
    Src/QHexEdit/QHexEdit.h \
    Src/QHexEdit/QHexEditPrivate.h \
    Src/QHexEdit/XByteArray.h \
    Src/Gui/PatchDialog.h \
    Src/Gui/PatchDialogGroupSelector.h \
    Src/Utils/UpdateChecker.h \
    Src/BasicView/SearchListViewTable.h \
    Src/Gui/CallStackView.h \
    Src/Gui/ShortcutsDialog.h \
    Src/BasicView/ShortcutEdit.h \
    Src/Gui/CalculatorDialog.h \
    Src/Gui/AttachDialog.h \
    Src/Gui/PageMemoryRights.h \
    Src/BasicView/HeaderButton.h \
    Src/ThirdPartyLibs/BeaEngine/basic_types.h \
    Src/ThirdPartyLibs/BeaEngine/BeaEngine.h \
    Src/ThirdPartyLibs/BeaEngine/export.h \
    Src/ThirdPartyLibs/BeaEngine/macros.h \
    Src/ThirdPartyLibs/OGDF/augmentation/DfsMakeBiconnected.h \
    Src/ThirdPartyLibs/OGDF/augmentation/PlanarAugmentation.h \
    Src/ThirdPartyLibs/OGDF/augmentation/PlanarAugmentationFix.h \
    Src/ThirdPartyLibs/OGDF/basic/AdjEntryArray.h \
    Src/ThirdPartyLibs/OGDF/basic/Array.h \
    Src/ThirdPartyLibs/OGDF/basic/Array2D.h \
    Src/ThirdPartyLibs/OGDF/basic/ArrayBuffer.h \
    Src/ThirdPartyLibs/OGDF/basic/Barrier.h \
    Src/ThirdPartyLibs/OGDF/basic/basic.h \
    Src/ThirdPartyLibs/OGDF/basic/BinaryHeap.h \
    Src/ThirdPartyLibs/OGDF/basic/BinaryHeap2.h \
    Src/ThirdPartyLibs/OGDF/basic/BoundedQueue.h \
    Src/ThirdPartyLibs/OGDF/basic/BoundedStack.h \
    Src/ThirdPartyLibs/OGDF/basic/CombinatorialEmbedding.h \
    Src/ThirdPartyLibs/OGDF/basic/comparer.h \
    Src/ThirdPartyLibs/OGDF/basic/Constraints.h \
    Src/ThirdPartyLibs/OGDF/basic/CriticalSection.h \
    Src/ThirdPartyLibs/OGDF/basic/DisjointSets.h \
    Src/ThirdPartyLibs/OGDF/basic/DualGraph.h \
    Src/ThirdPartyLibs/OGDF/basic/EdgeArray.h \
    Src/ThirdPartyLibs/OGDF/basic/EdgeComparer.h \
    Src/ThirdPartyLibs/OGDF/basic/EdgeComparerSimple.h \
    Src/ThirdPartyLibs/OGDF/basic/EFreeList.h \
    Src/ThirdPartyLibs/OGDF/basic/EList.h \
    Src/ThirdPartyLibs/OGDF/basic/exceptions.h \
    Src/ThirdPartyLibs/OGDF/basic/extended_graph_alg.h \
    Src/ThirdPartyLibs/OGDF/basic/FaceArray.h \
    Src/ThirdPartyLibs/OGDF/basic/FaceSet.h \
    Src/ThirdPartyLibs/OGDF/basic/geometry.h \
    Src/ThirdPartyLibs/OGDF/basic/Graph.h \
    Src/ThirdPartyLibs/OGDF/basic/Graph_d.h \
    Src/ThirdPartyLibs/OGDF/basic/graph_generators.h \
    Src/ThirdPartyLibs/OGDF/basic/GraphAttributes.h \
    Src/ThirdPartyLibs/OGDF/basic/GraphCopy.h \
    Src/ThirdPartyLibs/OGDF/basic/GraphCopyAttributes.h \
    Src/ThirdPartyLibs/OGDF/basic/GraphObserver.h \
    Src/ThirdPartyLibs/OGDF/basic/GridLayout.h \
    Src/ThirdPartyLibs/OGDF/basic/GridLayoutMapped.h \
    Src/ThirdPartyLibs/OGDF/basic/HashArray.h \
    Src/ThirdPartyLibs/OGDF/basic/HashArray2D.h \
    Src/ThirdPartyLibs/OGDF/basic/Hashing.h \
    Src/ThirdPartyLibs/OGDF/basic/HashIterator2D.h \
    Src/ThirdPartyLibs/OGDF/basic/HeapBase.h \
    Src/ThirdPartyLibs/OGDF/basic/HyperGraph.h \
    Src/ThirdPartyLibs/OGDF/basic/IncNodeInserter.h \
    Src/ThirdPartyLibs/OGDF/basic/Layout.h \
    Src/ThirdPartyLibs/OGDF/basic/List.h \
    Src/ThirdPartyLibs/OGDF/basic/Logger.h \
    Src/ThirdPartyLibs/OGDF/basic/Math.h \
    Src/ThirdPartyLibs/OGDF/basic/memory.h \
    Src/ThirdPartyLibs/OGDF/basic/MinHeap.h \
    Src/ThirdPartyLibs/OGDF/basic/MinPriorityQueue.h \
    Src/ThirdPartyLibs/OGDF/basic/Module.h \
    Src/ThirdPartyLibs/OGDF/basic/ModuleOption.h \
    Src/ThirdPartyLibs/OGDF/basic/NearestRectangleFinder.h \
    Src/ThirdPartyLibs/OGDF/basic/NodeArray.h \
    Src/ThirdPartyLibs/OGDF/basic/NodeComparer.h \
    Src/ThirdPartyLibs/OGDF/basic/NodeSet.h \
    Src/ThirdPartyLibs/OGDF/basic/precondition.h \
    Src/ThirdPartyLibs/OGDF/basic/PreprocessorLayout.h \
    Src/ThirdPartyLibs/OGDF/basic/Queue.h \
    Src/ThirdPartyLibs/OGDF/basic/simple_graph_alg.h \
    Src/ThirdPartyLibs/OGDF/basic/Skiplist.h \
    Src/ThirdPartyLibs/OGDF/basic/SList.h \
    Src/ThirdPartyLibs/OGDF/basic/Stack.h \
    Src/ThirdPartyLibs/OGDF/basic/String.h \
    Src/ThirdPartyLibs/OGDF/basic/System.h \
    Src/ThirdPartyLibs/OGDF/basic/Thread.h \
    Src/ThirdPartyLibs/OGDF/basic/Timeouter.h \
    Src/ThirdPartyLibs/OGDF/basic/TopologyModule.h \
    Src/ThirdPartyLibs/OGDF/basic/tuples.h \
    Src/ThirdPartyLibs/OGDF/basic/UMLGraph.h \
    Src/ThirdPartyLibs/OGDF/cluster/CconnectClusterPlanar.h \
    Src/ThirdPartyLibs/OGDF/cluster/CconnectClusterPlanarEmbed.h \
    Src/ThirdPartyLibs/OGDF/cluster/ClusterArray.h \
    Src/ThirdPartyLibs/OGDF/cluster/ClusterGraph.h \
    Src/ThirdPartyLibs/OGDF/cluster/ClusterGraphAttributes.h \
    Src/ThirdPartyLibs/OGDF/cluster/ClusterGraphCopyAttributes.h \
    Src/ThirdPartyLibs/OGDF/cluster/ClusterGraphObserver.h \
    Src/ThirdPartyLibs/OGDF/cluster/ClusterOrthoLayout.h \
    Src/ThirdPartyLibs/OGDF/cluster/ClusterOrthoShaper.h \
    Src/ThirdPartyLibs/OGDF/cluster/ClusterPlanarizationLayout.h \
    Src/ThirdPartyLibs/OGDF/cluster/ClusterPlanRep.h \
    Src/ThirdPartyLibs/OGDF/cluster/ClusterSet.h \
    Src/ThirdPartyLibs/OGDF/cluster/CPlanarEdgeInserter.h \
    Src/ThirdPartyLibs/OGDF/cluster/CPlanarSubClusteredGraph.h \
    Src/ThirdPartyLibs/OGDF/cluster/MaximumCPlanarSubgraph.h \
    Src/ThirdPartyLibs/OGDF/decomposition/BCTree.h \
    Src/ThirdPartyLibs/OGDF/decomposition/DynamicBCTree.h \
    Src/ThirdPartyLibs/OGDF/decomposition/DynamicPlanarSPQRTree.h \
    Src/ThirdPartyLibs/OGDF/decomposition/DynamicSkeleton.h \
    Src/ThirdPartyLibs/OGDF/decomposition/DynamicSPQRForest.h \
    Src/ThirdPartyLibs/OGDF/decomposition/DynamicSPQRTree.h \
    Src/ThirdPartyLibs/OGDF/decomposition/PertinentGraph.h \
    Src/ThirdPartyLibs/OGDF/decomposition/PlanarSPQRTree.h \
    Src/ThirdPartyLibs/OGDF/decomposition/Skeleton.h \
    Src/ThirdPartyLibs/OGDF/decomposition/SPQRTree.h \
    Src/ThirdPartyLibs/OGDF/decomposition/StaticPlanarSPQRTree.h \
    Src/ThirdPartyLibs/OGDF/decomposition/StaticSkeleton.h \
    Src/ThirdPartyLibs/OGDF/decomposition/StaticSPQRTree.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/BarycenterPlacer.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/CirclePlacer.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/EdgeCoverMerger.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/IndependentSetMerger.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/InitialPlacer.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/LocalBiconnectedMerger.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/MatchingMerger.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/MedianPlacer.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/MixedForceLayout.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/MMMExampleFastLayout.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/MMMExampleNiceLayout.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/MMMExampleNoTwistLayout.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/ModularMultilevelMixer.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/MultilevelBuilder.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/RandomMerger.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/RandomPlacer.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/ScalingLayout.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/SolarMerger.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/SolarPlacer.h \
    Src/ThirdPartyLibs/OGDF/energybased/multilevelmixer/ZeroPlacer.h \
    Src/ThirdPartyLibs/OGDF/energybased/CoinTutteLayout.h \
    Src/ThirdPartyLibs/OGDF/energybased/DavidsonHarel.h \
    Src/ThirdPartyLibs/OGDF/energybased/DavidsonHarelLayout.h \
    Src/ThirdPartyLibs/OGDF/energybased/FastMultipoleEmbedder.h \
    Src/ThirdPartyLibs/OGDF/energybased/FMMMLayout.h \
    Src/ThirdPartyLibs/OGDF/energybased/GEMLayout.h \
    Src/ThirdPartyLibs/OGDF/energybased/MultilevelLayout.h \
    Src/ThirdPartyLibs/OGDF/energybased/SpringEmbedderFR.h \
    Src/ThirdPartyLibs/OGDF/energybased/SpringEmbedderFRExact.h \
    Src/ThirdPartyLibs/OGDF/energybased/SpringEmbedderKK.h \
    Src/ThirdPartyLibs/OGDF/energybased/StressMajorizationSimple.h \
    Src/ThirdPartyLibs/OGDF/external/abacus.h \
    Src/ThirdPartyLibs/OGDF/external/coin.h \
    Src/ThirdPartyLibs/OGDF/fileformats/DinoLineBuffer.h \
    Src/ThirdPartyLibs/OGDF/fileformats/DinoTools.h \
    Src/ThirdPartyLibs/OGDF/fileformats/DinoUmlDiagramGraph.h \
    Src/ThirdPartyLibs/OGDF/fileformats/DinoUmlModelGraph.h \
    Src/ThirdPartyLibs/OGDF/fileformats/DinoUmlToGraphConverter.h \
    Src/ThirdPartyLibs/OGDF/fileformats/DinoXmlParser.h \
    Src/ThirdPartyLibs/OGDF/fileformats/DinoXmlScanner.h \
    Src/ThirdPartyLibs/OGDF/fileformats/GmlParser.h \
    Src/ThirdPartyLibs/OGDF/fileformats/Ogml.h \
    Src/ThirdPartyLibs/OGDF/fileformats/OgmlParser.h \
    Src/ThirdPartyLibs/OGDF/fileformats/simple_graph_load.h \
    Src/ThirdPartyLibs/OGDF/fileformats/SteinLibParser.h \
    Src/ThirdPartyLibs/OGDF/fileformats/XmlObject.h \
    Src/ThirdPartyLibs/OGDF/fileformats/XmlParser.h \
    Src/ThirdPartyLibs/OGDF/graphalg/CliqueFinder.h \
    Src/ThirdPartyLibs/OGDF/graphalg/Clusterer.h \
    Src/ThirdPartyLibs/OGDF/graphalg/ConvexHull.h \
    Src/ThirdPartyLibs/OGDF/graphalg/Dijkstra.h \
    Src/ThirdPartyLibs/OGDF/graphalg/GraphReduction.h \
    Src/ThirdPartyLibs/OGDF/graphalg/MinCostFlowReinelt.h \
    Src/ThirdPartyLibs/OGDF/graphalg/MinimumCut.h \
    Src/ThirdPartyLibs/OGDF/graphalg/PageRank.h \
    Src/ThirdPartyLibs/OGDF/graphalg/ShortestPathWithBFM.h \
    Src/ThirdPartyLibs/OGDF/internal/augmentation/PALabel.h \
    Src/ThirdPartyLibs/OGDF/internal/basic/intrinsics.h \
    Src/ThirdPartyLibs/OGDF/internal/basic/list_templates.h \
    Src/ThirdPartyLibs/OGDF/internal/basic/MallocMemoryAllocator.h \
    Src/ThirdPartyLibs/OGDF/internal/basic/PoolMemoryAllocator.h \
    Src/ThirdPartyLibs/OGDF/internal/cluster/basics.h \
    Src/ThirdPartyLibs/OGDF/internal/cluster/Cluster_ChunkConnection.h \
    Src/ThirdPartyLibs/OGDF/internal/cluster/Cluster_CutConstraint.h \
    Src/ThirdPartyLibs/OGDF/internal/cluster/Cluster_EdgeVar.h \
    Src/ThirdPartyLibs/OGDF/internal/cluster/Cluster_MaxPlanarEdges.h \
    Src/ThirdPartyLibs/OGDF/internal/cluster/ClusterPQContainer.h \
    Src/ThirdPartyLibs/OGDF/internal/cluster/CPlanarSubClusteredST.h \
    Src/ThirdPartyLibs/OGDF/internal/cluster/KuratowskiConstraint.h \
    Src/ThirdPartyLibs/OGDF/internal/cluster/MaxCPlanar_Master.h \
    Src/ThirdPartyLibs/OGDF/internal/cluster/MaxCPlanar_MinimalClusterConnection.h \
    Src/ThirdPartyLibs/OGDF/internal/cluster/MaxCPlanar_Sub.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/AdjacencyOracle.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/Attraction.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/EdgeAttributes.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/EnergyFunction.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/FruchtermanReingold.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/IntersectionRectangle.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/MultilevelGraph.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/NMM.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/NodeAttributes.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/NodePairEnergy.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/Overlap.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/ParticleInfo.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/Planarity.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/PlanarityGrid.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/QuadTreeNM.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/QuadTreeNodeNM.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/Repulsion.h \
    Src/ThirdPartyLibs/OGDF/internal/energybased/UniformGrid.h \
    Src/ThirdPartyLibs/OGDF/internal/lpsolver/LPSolver_coin.h \
    Src/ThirdPartyLibs/OGDF/internal/orthogonal/NodeInfo.h \
    Src/ThirdPartyLibs/OGDF/internal/orthogonal/RoutingChannel.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/BoyerMyrvoldInit.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/BoyerMyrvoldPlanar.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/ConnectedSubgraph.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/EmbedderMaxFaceBiconnectedGraphs.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/EmbedderMaxFaceBiconnectedGraphsLayers.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/EmbedIndicator.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/EmbedPQTree.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/FindKuratowskis.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/IndInfo.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/MaxSequencePQTree.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/MDMFLengthAttribute.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/PlanarLeafKey.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/PlanarPQTree.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/PlanarSubgraphPQTree.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/PQBasicKey.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/PQBasicKeyRoot.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/PQInternalKey.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/PQInternalNode.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/PQLeaf.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/PQLeafKey.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/PQNode.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/PQNodeKey.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/PQNodeRoot.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/PQTree.h \
    Src/ThirdPartyLibs/OGDF/internal/planarity/whaInfo.h \
    Src/ThirdPartyLibs/OGDF/internal/steinertree/EdgeWeightedGraph.h \
    Src/ThirdPartyLibs/OGDF/internal/steinertree/EdgeWeightedGraphCopy.h \
    Src/ThirdPartyLibs/OGDF/labeling/EdgeLabel.h \
    Src/ThirdPartyLibs/OGDF/labeling/ELabelInterface.h \
    Src/ThirdPartyLibs/OGDF/labeling/ELabelPosSimple.h \
    Src/ThirdPartyLibs/OGDF/layered/BarycenterHeuristic.h \
    Src/ThirdPartyLibs/OGDF/layered/CoffmanGrahamRanking.h \
    Src/ThirdPartyLibs/OGDF/layered/CrossingsMatrix.h \
    Src/ThirdPartyLibs/OGDF/layered/DfsAcyclicSubgraph.h \
    Src/ThirdPartyLibs/OGDF/layered/ExtendedNestingGraph.h \
    Src/ThirdPartyLibs/OGDF/layered/FastHierarchyLayout.h \
    Src/ThirdPartyLibs/OGDF/layered/FastSimpleHierarchyLayout.h \
    Src/ThirdPartyLibs/OGDF/layered/GreedyCycleRemoval.h \
    Src/ThirdPartyLibs/OGDF/layered/GreedyInsertHeuristic.h \
    Src/ThirdPartyLibs/OGDF/layered/GreedySwitchHeuristic.h \
    Src/ThirdPartyLibs/OGDF/layered/Hierarchy.h \
    Src/ThirdPartyLibs/OGDF/layered/Level.h \
    Src/ThirdPartyLibs/OGDF/layered/LongestPathRanking.h \
    Src/ThirdPartyLibs/OGDF/layered/MedianHeuristic.h \
    Src/ThirdPartyLibs/OGDF/layered/OptimalHierarchyClusterLayout.h \
    Src/ThirdPartyLibs/OGDF/layered/OptimalHierarchyLayout.h \
    Src/ThirdPartyLibs/OGDF/layered/OptimalRanking.h \
    Src/ThirdPartyLibs/OGDF/layered/SiftingHeuristic.h \
    Src/ThirdPartyLibs/OGDF/layered/SplitHeuristic.h \
    Src/ThirdPartyLibs/OGDF/layered/SugiyamaLayout.h \
    Src/ThirdPartyLibs/OGDF/lpsolver/LPSolver.h \
    Src/ThirdPartyLibs/OGDF/misclayout/BalloonLayout.h \
    Src/ThirdPartyLibs/OGDF/misclayout/CircularLayout.h \
    Src/ThirdPartyLibs/OGDF/misclayout/ProcrustesSubLayout.h \
    Src/ThirdPartyLibs/OGDF/module/AcyclicSubgraphModule.h \
    Src/ThirdPartyLibs/OGDF/module/AugmentationModule.h \
    Src/ThirdPartyLibs/OGDF/module/CCLayoutPackModule.h \
    Src/ThirdPartyLibs/OGDF/module/ClustererModule.h \
    Src/ThirdPartyLibs/OGDF/module/CPlanarSubgraphModule.h \
    Src/ThirdPartyLibs/OGDF/module/CrossingMinimizationModule.h \
    Src/ThirdPartyLibs/OGDF/module/EdgeInsertionModule.h \
    Src/ThirdPartyLibs/OGDF/module/EmbedderModule.h \
    Src/ThirdPartyLibs/OGDF/module/ForceLayoutModule.h \
    Src/ThirdPartyLibs/OGDF/module/FUPSModule.h \
    Src/ThirdPartyLibs/OGDF/module/GridLayoutModule.h \
    Src/ThirdPartyLibs/OGDF/module/HierarchyClusterLayoutModule.h \
    Src/ThirdPartyLibs/OGDF/module/HierarchyLayoutModule.h \
    Src/ThirdPartyLibs/OGDF/module/LayoutClusterPlanRepModule.h \
    Src/ThirdPartyLibs/OGDF/module/LayoutModule.h \
    Src/ThirdPartyLibs/OGDF/module/LayoutPlanRepModule.h \
    Src/ThirdPartyLibs/OGDF/module/MinCostFlowModule.h \
    Src/ThirdPartyLibs/OGDF/module/MixedModelCrossingsBeautifierModule.h \
    Src/ThirdPartyLibs/OGDF/module/MMCrossingMinimizationModule.h \
    Src/ThirdPartyLibs/OGDF/module/MMEdgeInsertionModule.h \
    Src/ThirdPartyLibs/OGDF/module/MultilevelLayoutModule.h \
    Src/ThirdPartyLibs/OGDF/module/PlanarityModule.h \
    Src/ThirdPartyLibs/OGDF/module/PlanarSubgraphModule.h \
    Src/ThirdPartyLibs/OGDF/module/RankingModule.h \
    Src/ThirdPartyLibs/OGDF/module/ShellingOrderModule.h \
    Src/ThirdPartyLibs/OGDF/module/ShortestPathModule.h \
    Src/ThirdPartyLibs/OGDF/module/TwoLayerCrossMin.h \
    Src/ThirdPartyLibs/OGDF/module/UMLLayoutModule.h \
    Src/ThirdPartyLibs/OGDF/module/UPRLayoutModule.h \
    Src/ThirdPartyLibs/OGDF/module/UpwardEdgeInserterModule.h \
    Src/ThirdPartyLibs/OGDF/module/UpwardPlanarizerModule.h \
    Src/ThirdPartyLibs/OGDF/module/UpwardPlanarSubgraphModule.h \
    Src/ThirdPartyLibs/OGDF/orthogonal/CompactionConstraintGraph.h \
    Src/ThirdPartyLibs/OGDF/orthogonal/EdgeRouter.h \
    Src/ThirdPartyLibs/OGDF/orthogonal/FlowCompaction.h \
    Src/ThirdPartyLibs/OGDF/orthogonal/LongestPathCompaction.h \
    Src/ThirdPartyLibs/OGDF/orthogonal/MinimumEdgeDistances.h \
    Src/ThirdPartyLibs/OGDF/orthogonal/OrthoLayout.h \
    Src/ThirdPartyLibs/OGDF/orthogonal/OrthoRep.h \
    Src/ThirdPartyLibs/OGDF/orthogonal/OrthoShaper.h \
    Src/ThirdPartyLibs/OGDF/packing/ComponentSplitterLayout.h \
    Src/ThirdPartyLibs/OGDF/packing/TileToRowsCCPacker.h \
    Src/ThirdPartyLibs/OGDF/planarity/BoothLueker.h \
    Src/ThirdPartyLibs/OGDF/planarity/BoyerMyrvold.h \
    Src/ThirdPartyLibs/OGDF/planarity/EdgeTypePatterns.h \
    Src/ThirdPartyLibs/OGDF/planarity/EmbedderMaxFace.h \
    Src/ThirdPartyLibs/OGDF/planarity/EmbedderMaxFaceLayers.h \
    Src/ThirdPartyLibs/OGDF/planarity/EmbedderMinDepth.h \
    Src/ThirdPartyLibs/OGDF/planarity/EmbedderMinDepthMaxFace.h \
    Src/ThirdPartyLibs/OGDF/planarity/EmbedderMinDepthMaxFaceLayers.h \
    Src/ThirdPartyLibs/OGDF/planarity/EmbedderMinDepthPiTa.h \
    Src/ThirdPartyLibs/OGDF/planarity/ExtractKuratowskis.h \
    Src/ThirdPartyLibs/OGDF/planarity/FastPlanarSubgraph.h \
    Src/ThirdPartyLibs/OGDF/planarity/FixedEmbeddingInserter.h \
    Src/ThirdPartyLibs/OGDF/planarity/KuratowskiSubdivision.h \
    Src/ThirdPartyLibs/OGDF/planarity/MaximalPlanarSubgraphSimple.h \
    Src/ThirdPartyLibs/OGDF/planarity/MaximumPlanarSubgraph.h \
    Src/ThirdPartyLibs/OGDF/planarity/MMFixedEmbeddingInserter.h \
    Src/ThirdPartyLibs/OGDF/planarity/MMSubgraphPlanarizer.h \
    Src/ThirdPartyLibs/OGDF/planarity/MMVariableEmbeddingInserter.h \
    Src/ThirdPartyLibs/OGDF/planarity/MultiEdgeApproxInserter.h \
    Src/ThirdPartyLibs/OGDF/planarity/NodeTypePatterns.h \
    Src/ThirdPartyLibs/OGDF/planarity/NonPlanarCore.h \
    Src/ThirdPartyLibs/OGDF/planarity/PlanarizationGridLayout.h \
    Src/ThirdPartyLibs/OGDF/planarity/PlanarizationLayout.h \
    Src/ThirdPartyLibs/OGDF/planarity/PlanRep.h \
    Src/ThirdPartyLibs/OGDF/planarity/PlanRepExpansion.h \
    Src/ThirdPartyLibs/OGDF/planarity/PlanRepInc.h \
    Src/ThirdPartyLibs/OGDF/planarity/PlanRepUML.h \
    Src/ThirdPartyLibs/OGDF/planarity/SimpleEmbedder.h \
    Src/ThirdPartyLibs/OGDF/planarity/SimpleIncNodeInserter.h \
    Src/ThirdPartyLibs/OGDF/planarity/SubgraphPlanarizer.h \
    Src/ThirdPartyLibs/OGDF/planarity/VariableEmbeddingInserter.h \
    Src/ThirdPartyLibs/OGDF/planarity/VariableEmbeddingInserter2.h \
    Src/ThirdPartyLibs/OGDF/planarlayout/BiconnectedShellingOrder.h \
    Src/ThirdPartyLibs/OGDF/planarlayout/FPPLayout.h \
    Src/ThirdPartyLibs/OGDF/planarlayout/MixedModelLayout.h \
    Src/ThirdPartyLibs/OGDF/planarlayout/MMCBBase.h \
    Src/ThirdPartyLibs/OGDF/planarlayout/MMCBDoubleGrid.h \
    Src/ThirdPartyLibs/OGDF/planarlayout/MMCBLocalStretch.h \
    Src/ThirdPartyLibs/OGDF/planarlayout/PlanarDrawLayout.h \
    Src/ThirdPartyLibs/OGDF/planarlayout/PlanarStraightLayout.h \
    Src/ThirdPartyLibs/OGDF/planarlayout/SchnyderLayout.h \
    Src/ThirdPartyLibs/OGDF/planarlayout/ShellingOrder.h \
    Src/ThirdPartyLibs/OGDF/planarlayout/TriconnectedShellingOrder.h \
    Src/ThirdPartyLibs/OGDF/simultaneous/SimDraw.h \
    Src/ThirdPartyLibs/OGDF/simultaneous/SimDrawCaller.h \
    Src/ThirdPartyLibs/OGDF/simultaneous/SimDrawColorizer.h \
    Src/ThirdPartyLibs/OGDF/simultaneous/SimDrawCreator.h \
    Src/ThirdPartyLibs/OGDF/simultaneous/SimDrawCreatorSimple.h \
    Src/ThirdPartyLibs/OGDF/simultaneous/SimDrawManipulatorModule.h \
    Src/ThirdPartyLibs/OGDF/simultaneous/TwoLayerCrossMinSimDraw.h \
    Src/ThirdPartyLibs/OGDF/tree/RadialTreeLayout.h \
    Src/ThirdPartyLibs/OGDF/tree/TreeLayout.h \
    Src/ThirdPartyLibs/OGDF/upward/DominanceLayout.h \
    Src/ThirdPartyLibs/OGDF/upward/ExpansionGraph.h \
    Src/ThirdPartyLibs/OGDF/upward/FaceSinkGraph.h \
    Src/ThirdPartyLibs/OGDF/upward/FeasibleUpwardPlanarSubgraph.h \
    Src/ThirdPartyLibs/OGDF/upward/FixedEmbeddingUpwardEdgeInserter.h \
    Src/ThirdPartyLibs/OGDF/upward/FixedUpwardEmbeddingInserter.h \
    Src/ThirdPartyLibs/OGDF/upward/FUPSSimple.h \
    Src/ThirdPartyLibs/OGDF/upward/LayerBasedUPRLayout.h \
    Src/ThirdPartyLibs/OGDF/upward/SubgraphUpwardPlanarizer.h \
    Src/ThirdPartyLibs/OGDF/upward/UpwardPlanarizationLayout.h \
    Src/ThirdPartyLibs/OGDF/upward/UpwardPlanarModule.h \
    Src/ThirdPartyLibs/OGDF/upward/UpwardPlanarSubgraphSimple.h \
    Src/ThirdPartyLibs/OGDF/upward/UpwardPlanRep.h \
    Src/ThirdPartyLibs/OGDF/upward/VisibilityLayout.h \
    Src/BasicView/FlowGraphScene.h \
    Src/BasicView/FlowGraphView.h \
    Src/BasicView/FlowGraphBlock.h \
    Src/BasicView/FlowGraphEdge.h \
    Src/Gui/FlowGraphWidget.h

INCLUDEPATH += \
    Src \
    Src/Gui \
    Src/BasicView \
    Src/Disassembler \
    Src/BeaEngine \
    Src/ThirdPartyLibs/BeaEngine \
    Src/Memory \
    Src/Bridge \
    Src/Global \
    Src/Utils \
    Src/ThirdPartyLibs

FORMS += \
    Src/Gui/MainWindow.ui \
    Src/Gui/CPUWidget.ui \
    Src/Gui/GotoDialog.ui \
    Src/Gui/WordEditDialog.ui \
    Src/Gui/LineEditDialog.ui \
    Src/Gui/SymbolView.ui \
    Src/BasicView/SearchListView.ui \
    Src/Gui/SettingsDialog.ui \
    Src/Gui/ExceptionRangeDialog.ui \
    Src/Gui/CommandHelpView.ui \
    Src/Gui/AppearanceDialog.ui \
    Src/Gui/CloseDialog.ui \
    Src/Gui/HexEditDialog.ui \
    Src/Gui/PatchDialog.ui \
    Src/Gui/PatchDialogGroupSelector.ui \
    Src/Gui/ShortcutsDialog.ui \
    Src/Gui/CalculatorDialog.ui \
    Src/Gui/AttachDialog.ui \
    Src/Gui/PageMemoryRights.ui

RESOURCES += \
    resource.qrc








