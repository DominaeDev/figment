#ifndef UNDO_STACK_H__
#define UNDO_STACK_H__
#pragma once

#include "Types.h"
#include "Hash.h"
#include <stack>
#include <optional>
#include <type_traits>
#include <concepts>

namespace fig
{
	template <typename T>
	concept IUndoState = requires (T const t)
	{
		{ t.GetHash() } -> std::same_as<size_t>;
		std::stack<T>().push(t);
	} and std::copyable<T>;

	template <IUndoState T>
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
		UndoStack(size_t capacity = 64uz) :
			_capacity { capacity }
		{}

		void PushState(const T& state)
		{
			// Compare hash (based on word count), and push to stack
			if (!_undoFrames.empty())
			{
				auto& top = _undoFrames.top();
				
				if (top.after.text == state.text || top.after.GetHash() == state.GetHash())
				{
					_stateBuffer[0] = state;
					return;
				}
			}
			
			_stateBuffer[1] = _stateBuffer[0];
			_stateBuffer[0] = state;
		}
		
		void SetInitState(const T& state)
		{
			_stateBuffer[0] = state;
			_stateBuffer[1] = state;
			stack_clear(_undoFrames);
			stack_clear(_redoFrames);
		}

		void MakeUndo()
		{
			stack_clear(_redoFrames);
			_undoFrames.push(UndoFrame {
				.before = _stateBuffer[1],
				.after = _stateBuffer[0],
			});
		}

		std::optional<T> Undo()
		{
			if (_undoFrames.empty())
				return std::nullopt;

			UndoFrame frame = _undoFrames.top();
			_undoFrames.pop();
			_redoFrames.push(frame);
			return frame.before;
		}

		std::optional<T> Redo()
		{
			if (_redoFrames.empty())
				return std::nullopt;
			UndoFrame frame = _redoFrames.top();
			_redoFrames.pop();
			_undoFrames.push(frame);
			return frame.after;
		}

		inline bool CanUndo() const noexcept { return !_undoFrames.empty(); }
		inline bool CanRedo() const noexcept { return !_redoFrames.empty(); }

	private:
		std::array<T, 2> _stateBuffer {};
		std::stack<UndoFrame> _undoFrames {};
		std::stack<UndoFrame> _redoFrames {};

		size_t _capacity = 64uz;
	};
}

#endif