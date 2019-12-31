/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2019 Inviwo Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************************/

#include <inviwo/tensorvisio/processors/vtkdatasettotensorfield3d.h>
#include <inviwo/vtk/util/vtkutil.h>
#include <inviwo/core/util/stringconversion.h>
#include <inviwo/core/network/networklock.h>

#include <warn/push>
#include <warn/ignore/all>
#include <vtkRectilinearGrid.h>
#include <vtkStructuredGrid.h>
#include <vtkStructuredPoints.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkArrayDispatch.h>
#include <warn/pop>

namespace inviwo {

// The Class Identifier has to be globally unique. Use a reverse DNS naming scheme
const ProcessorInfo VTKDataSetToTensorField3D::processorInfo_{
    "org.inviwo.VTKDataSetToTensorField3D",  // Class identifier
    "VTK Data Set To Tensor Field 3D",       // Display name
    "VTK",                                   // Category
    CodeState::Experimental,                 // Code state
    Tags::CPU,                               // Tags
};
const ProcessorInfo VTKDataSetToTensorField3D::getProcessorInfo() const { return processorInfo_; }

VTKDataSetToTensorField3D::VTKDataSetToTensorField3D()
    : Processor()
    , dataSetInport_("dataSetInport")
    , tensorField3DOutport_("tensorField3DOutport")
    , normalizeExtents_("normalizeExtents", "Normalize extents", false)
    , tensors_("tensors_", "Tensors")
    , scalars_("scalars", "Scalars")
    , generate_("generate", "Generate")
    , busy_(false) {
    addPort(dataSetInport_);
    addPort(tensorField3DOutport_);

    addProperty(normalizeExtents_);

    addProperty(tensors_);
    addProperty(scalars_);

    addProperties(generate_);

    dataSetInport_.onChange([this]() { initializeResources(); });

    generate_.onChange([this]() { generate(); });
}

void VTKDataSetToTensorField3D::initializeResources() {
    if (!dataSetInport_.hasData()) return;

    const auto& dataSet = *dataSetInport_.getData();
    auto pointData = dataSet->GetPointData();

    std::vector<OptionPropertyOption<std::string>> tensorOptions{};
    std::vector<OptionPropertyOption<std::string>> scalarOptions{};

    for (int i{0}; i < pointData->GetNumberOfArrays(); ++i) {
        auto array = pointData->GetArray(i);

        std::string name{array->GetName()};
        auto identifier = name;

        replaceInString(identifier, ".", "");
        replaceInString(identifier, " ", "");

        if (array->GetNumberOfComponents() == 9) {
            tensorOptions.emplace_back(identifier, name, name);
        }
        if (array->GetNumberOfComponents() == 1) {
            scalarOptions.emplace_back(identifier, name, name);
        }
    }

    scalarOptions.emplace_back("none", "None", "none");

    tensors_.replaceOptions(tensorOptions);
    scalars_.replaceOptions(scalarOptions);
}

void VTKDataSetToTensorField3D::process() {}

void VTKDataSetToTensorField3D::generate() {
    if (!dataSetInport_.hasData()) return;
    if (busy_) return;

    busy_ = true;

    dispatchPool([this]() {
        dispatchFront([this]() {
            {
                NetworkLock lock;
                tensors_.setReadOnly(true);
            }
            getActivityIndicator().setActive(true);
        });

        const auto& dataSet = *dataSetInport_.getData();

        auto tensorArray = dataSet->GetPointData()->GetArray(tensors_.get().c_str());

        if (!tensorArray) return;

        size3_t dimensions{0};

        if (const auto dimensionsOpt = dataSet.getDimensions()) {
            dimensions = *dimensionsOpt;
        } else {
            throw inviwo::Exception("Dimensions were not available.", IVW_CONTEXT);
        }

        LogProcessorInfo("Attempting to generate tensor field from array \""
                         << std::string{tensorArray->GetName()} << "\"");

        const auto bounds = dataSet->GetBounds();
        auto extent = vtkutil::extentFromBounds(bounds);
        const auto offset = vtkutil::offsetFromBounds(bounds);

        if (normalizeExtents_.get()) {
            extent /= std::max(std::max(extent.x, extent.y), extent.z);
        }

        std::shared_ptr<TensorField3D> tensorField;
        auto tensors = std::make_shared<std::vector<mat3>>();

        VTKToVector worker;

        typedef vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes> Dispatcher;

        if (!Dispatcher::Execute(tensorArray, worker)) {
            worker(tensorArray);
        }

        tensorField = std::make_shared<TensorField3D>(dimensions, worker.vec);
        tensorField->setExtents(extent);
        tensorField->setOffset(offset);

        busy_ = false;

        dispatchFront([this, tensorField]() {
            {
                NetworkLock lock;
                tensors_.setReadOnly(false);
            }
            getActivityIndicator().setActive(false);
            tensorField3DOutport_.setData(tensorField);
            invalidate(InvalidationLevel::InvalidOutput);
        });
    });
}

}  // namespace inviwo
