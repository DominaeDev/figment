#ifndef VARIABLE_LIST_H__
#define VARIABLE_LIST_H__

#pragma once

#include "gui/Control.h"
#include "gui/Graphics.h"

class StaticText;

class VariableList : public Control
{
public:
	VariableList(Control* pParent);
	void SetVariables(const std::map<string, string>& variables);
	bool IsEmpty() const;

protected:
	void OnRender(Renderer* pRenderer) override;

private:
	StaticText* _pText = nullptr;
};

#endif