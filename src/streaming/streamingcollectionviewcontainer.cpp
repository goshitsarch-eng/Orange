#include "streaming/streamingcollectionviewcontainer.h"

StreamingCollectionViewContainer::StreamingCollectionViewContainer(const std::string &title)
    : view_(std::make_unique<StreamingCollectionView>(title)) {}
