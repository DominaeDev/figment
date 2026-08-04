#pragma once

#include "Figment.h"
#include "EditorFields.h"

namespace fig::gui
{
	void CreateEditorField(ControlPtr pParent, fig::observer_ptr<Sizer> pSizer, const EditorField& field);
}