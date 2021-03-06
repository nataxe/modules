#pragma once

#include <warn/push>
#include <warn/ignore/all>
#include <vtkObject.h>
#include <vtkAlgorithm.h>
#include <warn/pop>

#include <inviwo/core/common/inviwoapplication.h>
#include <inviwo/core/processors/progressbar.h>

/**
 * Global method used as a callback for vtkCallbackCommand. The method needs to be global, otherwise
 * the signature will not match. clientData should be a pointer to the calling processor's
 * progressbar.
 * Note that you will need to explicitly set the client data via VTK's method SetClientData(void*)
 *
 * Example Usage:
 *
 *    auto progressCallback = vtkSmartPointer<vtkCallbackCommand>::New();
 *    progressCallback->SetCallback(vtkProgressBarCallback);
 *    progressCallback->SetClientData(&progressBar_);
 *
 *    auto someFilter = vtkSmartPointer<vtkSomeFilter>::New();
 *
 *    someFilter->AddObserver(vtkCommand::ProgressEvent, progressCallback);
 *
 */
inline void vtkProgressBarCallback(vtkObject* caller, long unsigned int, void* clientData, void*) {

    if (auto algorithm = dynamic_cast<vtkAlgorithm*>(caller)) {

        auto progressBar = static_cast<inviwo::ProgressBar*>(clientData);

        if (progressBar) {

            if (algorithm->GetProgress() > 0.0) {
                const auto f = float(algorithm->GetProgress());

                inviwo::dispatchFront([progressBar, f]() {
                    f < 1.0f ? progressBar->show() : progressBar->hide();
                    progressBar->updateProgress(f);
                });
            }
        }
    }
}

namespace inviwo {
namespace vtkutil {
inline dvec3 extentFromBounds(const double* bounds) {
    return dvec3{bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4]};
}

inline dvec3 offsetFromBounds(const double* bounds) {
    return dvec3{bounds[0], bounds[2], bounds[4]};
}
}  // namespace vtkutil
}  // namespace inviwo
