export module GUI.Controls.VariableList;
export import GUI.Control;

import Common;
import GUI.Controls.StaticText;
import GUI.GraphicTypes;
import NineGridBackgroundRenderer;
import TextureStore;

constexpr float Margin = 8.0f;

export
{
	class VariableList : public Control
	{
	public:
		VariableList(Control* pParent)
			: Control(pParent)
		{
			auto pBG = new NineGridBackgroundRenderer({ 30, 72, 64, 30 });
			pBG->SetTextures(TextureStore::GetTexture(TextureType::SPEECH_BUBBLE_CENTER_BG), TextureStore::GetTexture(TextureType::SPEECH_BUBBLE_CENTER_BORDER));
			pBG->SetColors(Colors::MessageBackgroundDefault, Colors::MessageBorderDefault);
			pBG->SetCornerSize(6);
			SetBackgroundRenderer(pBG);

			SetForegroundColor(Colors::TextForeground);
			SetBackgroundColor(Colors::MessageBackgroundDefault);

			_pText = new StaticText(this, "", FontFace::Default, Constants::GUI::StatusBarFontSize, false);
			_pText->SetPosition(Margin, Margin);
			_pText->SetMaxSize(250, -1);
		}

		void SetVariables(const std::map<string, string>& variables)
		{
			string text;
			text.reserve(512);
			for (auto kvp : variables)
				text = text + std::format("{} = {}\n", kvp.first, kvp.second);
			text = string_util::rtrim(text);

			int w, h;
			_pText->SetTextAndResize(text, w, h);

			SetSize(toF(w + Margin * 2), toF(h + Margin * 2));
		}

		bool IsEmpty() const
		{
			return _pText->GetText().empty();
		}

	protected:
		void OnRender(Renderer* pRenderer) override
		{
			if (_pText->GetText().empty())
				return;
			Control::OnRender(pRenderer);
		}

	private:
		StaticText* _pText = nullptr;
	};
}