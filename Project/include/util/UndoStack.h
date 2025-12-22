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
	/// <summary>
	/// Concept: A custom undo state struct type that's trivially copyable and has the following members:
	///		enum actionType
	///		bool mayCoalesce
	/// </summary>
	template <typename T, typename EType>
	concept IUndoState = requires (T const t)
	{
		{ t.actionType } -> std::same_as<const EType&>;
		{ t.mayCoalesce } -> std::same_as<const bool&>;
		std::stack<T>().push(t);
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
		UndoStack(size_t capacity = 64uz) :
			_capacity { capacity }
		{}
		
		void SetInitState(const T& state)
		{
			_stateBuffer[0] = state;
			_stateBuffer[1] = state;
			stack_clear(_undoFrames);
			stack_clear(_redoFrames);
		}

		void PushState(const T& state)
		{
			_stateBuffer[1] = _stateBuffer[0];
			_stateBuffer[0] = state;
		}

		void CreateUndo()
		{
			stack_clear(_redoFrames);

			if (!_undoFrames.empty())
			{
				// Coalesce?
				auto& topFrame = _undoFrames.top();
				auto& lastState = _stateBuffer[0];
				bool bCoalesce = topFrame.after.mayCoalesce && lastState.mayCoalesce;
				if (!_undoFrames.empty() and bCoalesce)
				{
					bCoalesce &= topFrame.after.actionType == lastState.actionType;
					if (bCoalesce)
					{
						topFrame.after = lastState;
						return;
					}
				}
			}

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

			_stateBuffer[0] = frame.before;
			return frame.before;
		}

		std::optional<T> Redo()
		{
			if (_redoFrames.empty())
				return std::nullopt;

			UndoFrame frame = _redoFrames.top();
			_redoFrames.pop();
			_undoFrames.push(frame);

			_stateBuffer[0] = frame.after;
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