#include <inviwo/tensorvisbase/datastructures/tensorfield3d.h>
#include <inviwo/core/datastructures/volume/volumeram.h>
#include <inviwo/core/util/stdextensions.h>
#include <modules/eigenutils/eigenutils.h>
#include <inviwo/tensorvisbase/util/misc.h>
#include <inviwo/core/util/exception.h>

namespace inviwo {

TensorField3D::TensorField3D(const size3_t &dimensions, const std::vector<mat3> &tensors)
    : dimensions_(dimensions)
    , indexMapper_(util::IndexMapper3D(dimensions))
    , tensors_(std::make_shared<std::vector<mat3>>(tensors))
    , size_(glm::compMul(dimensions))
    , metaData_(std::make_shared<DataFrame>()) {
    initializeDefaultMetaData();
}

TensorField3D::TensorField3D(const size3_t &dimensions,
                             std::shared_ptr<const std::vector<mat3>> tensors)
    : dimensions_(dimensions)
    , indexMapper_(util::IndexMapper3D(dimensions))
    , tensors_(tensors)
    , size_(glm::compMul(dimensions))
    , metaData_(std::make_shared<DataFrame>()) {
    initializeDefaultMetaData();
}

TensorField3D::TensorField3D(const size3_t &dimensions, const std::vector<mat3> &tensors,
                             const DataFrame &metaData)
    : dimensions_(dimensions)
    , indexMapper_(util::IndexMapper3D(dimensions))
    , tensors_(std::make_shared<std::vector<mat3>>(tensors))
    , size_(glm::compMul(dimensions))
    , metaData_(std::make_shared<DataFrame>(metaData)) {
    initializeDefaultMetaData();
}

TensorField3D::TensorField3D(const size3_t &dimensions,
                             std::shared_ptr<const std::vector<mat3>> tensors,
                             std::shared_ptr<const DataFrame> metaData)
    : dimensions_(dimensions)
    , indexMapper_(util::IndexMapper3D(dimensions))
    , tensors_(tensors)
    , size_(glm::compMul(dimensions))
    , metaData_(metaData) {
    initializeDefaultMetaData();
}

TensorField3D::TensorField3D(const TensorField3D &tf)
    : StructuredGridEntity<3>()
    , dataMapEigenValues_(tf.dataMapEigenValues_)
    , dataMapEigenVectors_(tf.dataMapEigenVectors_)
    , dimensions_(tf.dimensions_)
    , indexMapper_(util::IndexMapper3D(dimensions_))
    , tensors_(tf.tensors_)
    , size_(tf.size_)
    , rank_(tf.rank_)
    , dimensionality_(tf.dimensionality_)
    , metaData_(tf.metaData_)
    , binaryMask_(tf.binaryMask_) {
    setOffset(tf.getOffset());
    setBasis(tf.getBasis());
}

std::string TensorField3D::getDataInfo() const {
    std::stringstream ss;
    ss << "<table border='0' cellspacing='0' cellpadding='0' "
          "style='border-color:white;white-space:pre;'>/n"
       << tensorutil::getHTMLTableRowString("Type", "3D tensor field")
       << tensorutil::getHTMLTableRowString("Number of tensors", tensors_->size())
       << tensorutil::getHTMLTableRowString("Dimensions", dimensions_)
       << tensorutil::getHTMLTableRowString("Max major field eigenvalue",
                                            dataMapEigenValues_[0].valueRange.y)
       << tensorutil::getHTMLTableRowString("Min major field eigenvalue",
                                            dataMapEigenValues_[0].valueRange.x)
       << tensorutil::getHTMLTableRowString("Max intermediate field eigenvalue",
                                            dataMapEigenValues_[1].valueRange.y)
       << tensorutil::getHTMLTableRowString("Min intermediate field eigenvalue",
                                            dataMapEigenValues_[1].valueRange.x)
       << tensorutil::getHTMLTableRowString("Max minor field eigenvalue",
                                            dataMapEigenValues_[2].valueRange.y)
       << tensorutil::getHTMLTableRowString("Min minor field eigenvalue",
                                            dataMapEigenValues_[2].valueRange.x)
       << tensorutil::getHTMLTableRowString("Extends", getExtents())

       << "</table>";
    return ss.str();
}

/**
 *   Returns three volumes representing the tensor field.
 *
 * At each position, a tensor is given by

 *   xx   xy   xz
 *   yx   yy   yz
 *   zx   zy   zz
 *
 * The volumes return by this method decompose the tensor into its three columns.
 * I.e., the first volume contains the values xx, yz, and zx.
 */
std::array<std::shared_ptr<Volume>, 3> TensorField3D::getVolumeRepresentation() const {
    auto volumeCol1 = std::make_shared<Volume>(dimensions_, DataVec3Float32::get());
    auto volumeCol2 = std::make_shared<Volume>(dimensions_, DataVec3Float32::get());
    auto volumeCol3 = std::make_shared<Volume>(dimensions_, DataVec3Float32::get());

    auto col1RAM = volumeCol1->getEditableRepresentation<VolumeRAM>();
    auto col2RAM = volumeCol2->getEditableRepresentation<VolumeRAM>();
    auto col3RAM = volumeCol3->getEditableRepresentation<VolumeRAM>();

    util::IndexMapper3D indexMapper(dimensions_);

    for (size_t z = 0; z < dimensions_.z; z++) {
        for (size_t x = 0; x < dimensions_.x; x++) {
            for (size_t y = 0; y < dimensions_.y; y++) {
                const auto &tensor = at(size3_t(x, y, z));

                col1RAM->setFromDVec3(size3_t(x, y, z), tensor[0]);

                col2RAM->setFromDVec3(size3_t(x, y, y), tensor[1]);

                col3RAM->setFromDVec3(size3_t(x, y, y), tensor[2]);
            }
        }
    }

    return {volumeCol1, volumeCol2, volumeCol3};
}

void TensorField3D::setExtents(const vec3 &extents) {
    auto basis = getBasis();
    basis[0] = glm::normalize(basis[0]) * extents[0];
    basis[1] = glm::normalize(basis[1]) * extents[1];
    basis[2] = glm::normalize(basis[2]) * extents[2];
    setBasis(basis);
}

mat4 TensorField3D::getBasisAndOffset() const {
    auto basis = getBasis();
    auto offset = getOffset();

    mat4 modelMatrix;

    modelMatrix[0][0] = basis[0][0];
    modelMatrix[0][1] = basis[0][1];
    modelMatrix[0][2] = basis[0][2];

    modelMatrix[1][0] = basis[1][0];
    modelMatrix[1][1] = basis[1][1];
    modelMatrix[1][2] = basis[1][2];

    modelMatrix[2][0] = basis[2][0];
    modelMatrix[2][1] = basis[2][1];
    modelMatrix[2][2] = basis[2][2];

    modelMatrix[3][0] = offset[0];
    modelMatrix[3][1] = offset[1];
    modelMatrix[3][2] = offset[2];

    modelMatrix[0][3] = 0.0f;
    modelMatrix[1][3] = 0.0f;
    modelMatrix[2][3] = 0.0f;
    modelMatrix[3][3] = 1.0f;

    return modelMatrix;
}

const std::vector<vec3> &TensorField3D::majorEigenVectors() const {
    return this->getMetaDataContainer<attributes::MajorEigenVector>();
}

const std::vector<vec3> &TensorField3D::intermediateEigenVectors() const {
    return this->getMetaDataContainer<attributes::IntermediateEigenVector>();
}

const std::vector<vec3> &TensorField3D::minorEigenVectors() const {
    return this->getMetaDataContainer<attributes::MinorEigenVector>();
}

const std::vector<float> &TensorField3D::majorEigenValues() const {
    return this->getMetaDataContainer<attributes::Lambda1>();
}

const std::vector<float> &TensorField3D::intermediateEigenValues() const {
    return this->getMetaDataContainer<attributes::Lambda2>();
}

const std::vector<float> &TensorField3D::minorEigenValues() const {
    return this->getMetaDataContainer<attributes::Lambda3>();
}

std::shared_ptr<const std::vector<mat3>> TensorField3D::tensors() const { return tensors_; }

void TensorField3D::setTensors(std::shared_ptr<const std::vector<mat3>> tensors) {
    tensors_ = tensors;
}

void TensorField3D::setMetaData(std::shared_ptr<const DataFrame> metaData) { metaData_ = metaData; }

int TensorField3D::getNumDefinedEntries() const {
    return static_cast<int>(std::count(std::begin(binaryMask_), std::end(binaryMask_), 1));
}

std::shared_ptr<TensorField3D> TensorField3D::deepCopy() const {
    auto tf = std::make_shared<TensorField3D>(*this);

    tf->setTensors(std::make_shared<std::vector<mat3>>(*tensors_));
    tf->setMetaData(std::make_shared<DataFrame>(*metaData_));

    return tf;
}

TensorField3D *TensorField3D::clone() const { return new TensorField3D(*this); }

void TensorField3D::initializeDefaultMetaData() {
    // clang-format off
    if (this->hasMetaData<attributes::Lambda1>() &&
        this->hasMetaData<attributes::Lambda2>() &&
        this->hasMetaData<attributes::Lambda3>() &&
        this->hasMetaData<attributes::MajorEigenVector>() &&
        this->hasMetaData<attributes::IntermediateEigenVector>() &&
        this->hasMetaData<attributes::MinorEigenVector>()) {
        return;
    }
    // clang-format on

    auto newMetaData = std::make_shared<DataFrame>(*metaData_);

    auto func = [](const mat3 &tensor) -> std::array<std::pair<float, vec3>, 3> {
        if (tensor == mat3(0.0f)) {
            return {{std::make_pair(0.0f, vec3{0.0f}), std::make_pair(0.0f, vec3{0.0f}),
                     std::make_pair(0.0f, vec3{0.0f})}};
            return std::array<std::pair<float, vec3>, 3>{std::make_pair(0.0f, vec3(0.0f)),
                                                         std::make_pair(0.0f, vec3(0.0f)),
                                                         std::make_pair(0.0f, vec3(0.0f))};
        }

        Eigen::EigenSolver<Eigen::Matrix3f> solver(util::glm2eigen(tensor));
        const auto eigenValues = util::eigen2glm<float, 3, 1>(solver.eigenvalues().real());
        const auto eigenVectors = util::eigen2glm<float, 3, 3>(solver.eigenvectors().real());

        const auto range =
            util::as_range(glm::value_ptr(eigenValues), glm::value_ptr(eigenValues) + 3);

        const auto ordering = util::ordering(range, std::greater<float>());

        return {{std::make_pair(eigenValues[ordering[0]], eigenVectors[ordering[0]]),
                 std::make_pair(eigenValues[ordering[1]], eigenVectors[ordering[1]]),
                 std::make_pair(eigenValues[ordering[2]], eigenVectors[ordering[2]])}};
    };

    std::vector<float> majorEigenValues;
    std::vector<float> intermediateEigenValues;
    std::vector<float> minorEigenValues;

    std::vector<vec3> majorEigenVectors;
    std::vector<vec3> intermediateEigenVectors;
    std::vector<vec3> minorEigenVectors;

    majorEigenValues.resize(size_);
    intermediateEigenValues.resize(size_);
    minorEigenValues.resize(size_);

    majorEigenVectors.resize(size_);
    intermediateEigenVectors.resize(size_);
    minorEigenVectors.resize(size_);

    const auto &t_ref = *tensors_;

#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(tensors_->size()); i++) {
        auto tensor = t_ref[i];
        auto eigenValuesAndEigenVectors = func(tensor);

        majorEigenVectors[i] = eigenValuesAndEigenVectors[0].second;
        intermediateEigenVectors[i] = eigenValuesAndEigenVectors[1].second;
        minorEigenVectors[i] = eigenValuesAndEigenVectors[2].second;

        majorEigenValues[i] = eigenValuesAndEigenVectors[0].first;
        intermediateEigenValues[i] = eigenValuesAndEigenVectors[1].first;
        minorEigenValues[i] = eigenValuesAndEigenVectors[2].first;
    }

    addIfNotPresent<attributes::Lambda1>(newMetaData, majorEigenValues);
    addIfNotPresent<attributes::Lambda2>(newMetaData, intermediateEigenValues);
    addIfNotPresent<attributes::Lambda3>(newMetaData, minorEigenValues);
    addIfNotPresent<attributes::MajorEigenVector>(newMetaData, majorEigenVectors);
    addIfNotPresent<attributes::IntermediateEigenVector>(newMetaData, intermediateEigenVectors);
    addIfNotPresent<attributes::MinorEigenVector>(newMetaData, minorEigenVectors);

    metaData_ = newMetaData;
}

vec3 TensorField3D::getNormalizedVolumePosition(const size_t index, const double sliceCoord) const {
    auto stepSize = getSpacing();
    const auto pos = indexMapper_(index);
    return vec3(dimensions_.x < 2 ? sliceCoord : pos.x * stepSize.x,
                dimensions_.y < 2 ? sliceCoord : pos.y * stepSize.y,
                dimensions_.z < 2 ? sliceCoord : pos.z * stepSize.z);
}

std::optional<std::vector<vec3>> TensorField3D::getNormalizedScreenCoordinates(float sliceCoord) {
    std::vector<vec3> normalizedVolumePositions;
    normalizedVolumePositions.resize(size_);

    auto stepSize = getSpacing();

    if (dimensions_.x == 0 || dimensions_.y == 0 || dimensions_.z == 0) {
        LogError("Tensor field 3D has at least one zero-sized dimension!");
        return std::nullopt;
    }

    for (size_t z = 0; z < dimensions_.z; z++) {
        for (size_t y = 0; y < dimensions_.y; y++) {
            for (size_t x = 0; x < dimensions_.x; x++) {
                const auto index = indexMapper_(size3_t(x, y, z));
                normalizedVolumePositions[index] = getNormalizedVolumePosition(index, sliceCoord);
            }
        }
    }

    return normalizedVolumePositions;
}

void TensorField3D::computeDataMaps() {
    const auto &majorEigenValues = this->majorEigenValues();
    const auto &middleEigenValues = this->intermediateEigenValues();
    const auto &minorEigenValues = this->minorEigenValues();

    dataMapEigenValues_[0].dataRange.x = dataMapEigenValues_[0].valueRange.x =
        *std::min_element(majorEigenValues.begin(), majorEigenValues.end());
    dataMapEigenValues_[0].dataRange.y = dataMapEigenValues_[0].valueRange.y =
        *std::max_element(majorEigenValues.begin(), majorEigenValues.end());
    dataMapEigenValues_[1].dataRange.x = dataMapEigenValues_[1].valueRange.x =
        *std::min_element(middleEigenValues.begin(), middleEigenValues.end());
    dataMapEigenValues_[1].dataRange.y = dataMapEigenValues_[1].valueRange.y =
        *std::max_element(middleEigenValues.begin(), middleEigenValues.end());
    dataMapEigenValues_[2].dataRange.x = dataMapEigenValues_[2].valueRange.x =
        *std::min_element(minorEigenValues.begin(), minorEigenValues.end());
    dataMapEigenValues_[2].dataRange.y = dataMapEigenValues_[2].valueRange.y =
        *std::max_element(minorEigenValues.begin(), minorEigenValues.end());

    auto min_f = [](const dvec3 &vec) -> double { return std::min(std::min(vec.x, vec.y), vec.z); };
    auto max_f = [](const dvec3 &vec) -> double { return std::max(std::max(vec.x, vec.y), vec.z); };

    auto min = std::numeric_limits<double>::max();
    auto max = std::numeric_limits<double>::lowest();
    for (const auto &v : this->majorEigenVectors()) {
        min = std::min(min_f(v), min);
        max = std::max(max_f(v), max);
    }
    dataMapEigenVectors_[0].dataRange.x = dataMapEigenVectors_[0].valueRange.x = min;
    dataMapEigenVectors_[0].dataRange.y = dataMapEigenVectors_[0].valueRange.y = max;

    min = std::numeric_limits<double>::max();
    max = std::numeric_limits<double>::lowest();
    for (const auto &v : this->intermediateEigenVectors()) {
        min = std::min(min_f(v), min);
        max = std::max(max_f(v), max);
    }
    dataMapEigenVectors_[1].dataRange.x = dataMapEigenVectors_[1].valueRange.x = min;
    dataMapEigenVectors_[1].dataRange.y = dataMapEigenVectors_[1].valueRange.y = max;

    min = std::numeric_limits<double>::max();
    max = std::numeric_limits<double>::lowest();
    for (const auto &v : this->minorEigenVectors()) {
        min = std::min(min_f(v), min);
        max = std::max(max_f(v), max);
    }
    dataMapEigenVectors_[2].dataRange.x = dataMapEigenVectors_[2].valueRange.x = min;
    dataMapEigenVectors_[2].dataRange.y = dataMapEigenVectors_[2].valueRange.y = max;
}

}  // namespace inviwo
