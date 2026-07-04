#pragma once

#include "Figment.h"
#include "Hash.h"
#include <stack>
#include <list>
#include <optional>
#include <type_traits>
#include <concepts>

namespace fig
{
	/// <summary>
	/// Concept: A custom undo state struct type that's trivially copyable and has the following members:
	///		enum actionType
	/// </summary>
	template <typename T, typename EType>
	concept IUndoState = requires (T const t)
	{
		{ t.actionType } -> std::same_as<const EType&>;
		std::stack<T>().push(t);
		std::list<T>().push_front(t);
	} and std::copyable<T>;

	/// <summary>
	/// Generic undo stack
	/// </summary>
	template <typename T, typename EType>
		requires IUndoState<T, EType>
	class UndoStack
	{
		struct UndoFrame
		{
			T before;
			T after;
		};

		template<typename T>
		void stack_clear(std::stack<T>& q)
		{
			std::stack<T> empty;
			std::swap(q, empty);
		}
	public:
		UndoStack(size_t capacity = 512uz) :
			_capacity { capacity }
		{}
		
		void SetInitState(const T& state)
		{
			_stateBuffer[0] = state;
			_stateBuffer[1] = state;
			_undoFrames.clear();
			stack_clear(_redoFrames);
		}

		void PushState(const T& state)
		{
			_stateBuffer[1] = _stateBuffer[0];
			_stateBuffer[0] = state;
		}

		void CreateUndo(bool allowCoalesce = true)
		{
			stack_clear(_redoFrames);

			// Coalesce
			if (!_undoFrames.empty() && allowCoalesce)
			{
				auto& topFrame = _undoFrames.front();
				auto& lastState = _stateBuffer[0];
				if (topFrame.after.actionType == lastState.actionType)
				{
					topFrame.after = lastState;
					return;
				}
			}

			_undoFrames.push_front(UndoFrame {
				.before = _stateBuffer[1],
				.after = _stateBuffer[0],
			});
			while (_undoFrames.size() > _capacity)
				_undoFrames.pop_back();
		}

		std::optional<T> Undo()
		{
			if (_undoFrames.empty())
				return std::nullopt;

			UndoFrame frame = _undoFrames.front();
			_undoFrames.pop_front();
			_redoFrames.push(frame);

			_stateBuffer[0] = frame.before;
			return frame.before;
		}

		std::optional<T> Redo()
		{
			if (_redoFrames.empty())
				return std::nullopt;

			UndoFrame frame = _redoFrames.top();
			_redoFrames.pop();
			_undoFrames.push_front(frame);

			_stateBuffer[0] = frame.after;
			return frame.after;
		}

		inline bool CanUndo() const noexcept { return !_undoFrames.empty(); }
		inline bool CanRedo() const noexcept { return !_redoFrames.empty(); }

	private:
		std::array<T, 2> _stateBuffer {};
		std::list<UndoFrame> _undoFrames {};
		std::stack<UndoFrame> _redoFrames {};
		size_t _capacity = 512uz;
	};
}
