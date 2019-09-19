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

#ifndef IVW_CONTOURTREECOLORMAPPER_H
#define IVW_CONTOURTREECOLORMAPPER_H

#include <inviwo/topologytoolkit/topologytoolkitmoduledefine.h>
#include <inviwo/topologytoolkit/ports/triangulationdataport.h>

#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/processors/processor.h>
#include <inviwo/core/ports/imageport.h>
#include <inviwo/core/ports/meshport.h>
#include <inviwo/core/properties/ordinalproperty.h>
#include <inviwo/core/properties/optionproperty.h>
#include <inviwo/core/properties/compositeproperty.h>
#include <inviwo/core/properties/boolproperty.h>
#include <inviwo/core/properties/minmaxproperty.h>
#include <inviwo/core/interaction/pickingmapper.h>
#include <inviwo/dataframe/datastructures/dataframe.h>
#include <inviwo/core/properties/buttonproperty.h>
#include <inviwo/core/properties/transferfunctionproperty.h>


#include <inviwo/topologytoolkit/ports/contourtreeport.h>
#include <inviwo/core/datastructures/geometry/mesh.h>
#include <inviwo/core/datastructures/geometry/meshram.h>
#include <inviwo/topologytoolkit/utils/ttkutils.h>
#include <inviwo/topologytoolkit/utils/ttkexception.h>


#include <warn/push>
#include <warn/ignore/all>
#include <ttk/core/base/ftmTree/FTMTree.h>
#include <warn/pop>

namespace inviwo {

namespace topology {

enum MeshColorOption {
	SCALAR_COLORMAP = 0,
	SEGMENT_COLORMAP
};

std::shared_ptr<Mesh> mapMeshToContourTree(const TransferFunction& tf, const Mesh& mesh,
                                           const TriangulationData& triangulationData,
											MeshColorOption coloroption = MeshColorOption::SEGMENT_COLORMAP);

}
class IVW_MODULE_TOPOLOGYTOOLKIT_API ContourTreeColorMapper : public Processor {
public:
    ContourTreeColorMapper();
    virtual ~ContourTreeColorMapper() = default;

    virtual void process() override;

    virtual const ProcessorInfo getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;

private:
    MeshInport meshInport_;
    topology::TriangulationInport triangulationInport;
    topology::ContourTreeInport contourtreeInport;
    MeshOutport outport_;

	TransferFunctionProperty transferFunction_;
};

}  // namespace inviwo

#endif  // IVW_CONTOURTREECOLORMAPPER_H