#pragma once

#include <vector>
#include <map>
#include <memory>
#include <algorithm>

namespace MlActions {
	template<typename T>
	struct EventListener;

	template<typename T, typename... Args>
	struct EventListener<T(Args...)> {
		std::map<size_t, std::function<T(Args...)>> listeners;
		/// current id
		size_t id = 0;

		size_t add (std::function<T(Args...)> listener) {
			listeners[id] = listener;
			const size_t ret_id = id;
			id++;
			return ret_id;
		}

		void remove (size_t rid) {
			listeners.erase(rid);
		}

		/// Calls [run]
		void operator() (Args... args) {
			run(args...);
		}

		/// Runs every listener with provided args
		void run (Args... args) {
			for (auto& listener : listeners) {
				listener.second(args...);
			}
		}
	};
	template<typename... Args>
	struct EventListenerCheck : public EventListener<bool(Args...)> {
		/// Early returns at the first `false` return
		bool check (Args... args) {
			for (auto& listener : this->listeners) {
				if (!listener(args...)) {
					return false;
				}
			}
			return true;
		}
	};


	class Action {
		public:
		explicit Action () {}
		virtual ~Action () {}

		/// Called whenever the Action is undone
		virtual void undo () = 0;
		/// Called whenever the Action is redone
		/// As well as the first time it is 'done'
		virtual void redo () = 0;

		/// Returns whether this Action can be undone.
		/// Some actions are permanent, and so thus can't be undone whatsoever
		virtual bool canUndo () const {
			return true;
		}
	};

	class BundledAction : public Action {
		public:
		std::vector<std::unique_ptr<Action>> actions;

		explicit BundledAction () : Action() {}

		void add (std::unique_ptr<Action>&& action) {
			actions.push_back(std::move(action));
		}

		bool canUndo () const override {
			for (auto& action : actions) {
				if (!action->canUndo()) {
					return false;
				}
			}
			return true;
		}

		void undo () override {
			for (std::unique_ptr<Action>& action : actions) {
				action->undo();
			}
		}

		void redo () override {
			for (std::unique_ptr<Action>& action : actions) {
				action->redo();
			}
		}
	};

	namespace detail {
		template<typename F>
		static void visitActions (F&& func, std::vector<std::unique_ptr<Action>>& actions) {
			for (std::unique_ptr<Action>& action : actions) {
				func(action);
			}
		}

		template<typename F>
		static void visitAction (F&& func, std::unique_ptr<Action>& action) {
			if (BundledAction* bundled_action = dynamic_cast<BundledAction*>(action.get())) {
				visitActions(func, bundled_action->actions);
			} else {
				func(action);
			}
		}
	}

	class ActionList {
		public:
		/// One should be careful of modifying this.
		std::vector<std::unique_ptr<Action>> actions;
		/// Any action <  position is 'past'
		/// Any action >=  position is 'future'
		/// position == actions.size() when there is no future actions
		/// position == 0              when there is no past   actions
		size_t position = 0;

		/// Called after future is cleared
		EventListener<void()> clearFutureListener;
		/// Called before action is added (or 're'done)
		EventListener<void(std::unique_ptr<Action>&)> addListener;

		public:

		explicit ActionList () {}

		/// throws away everything after current position
		/// adds action
		/// and calls redo on action
		void add (std::unique_ptr<Action>&& action) {
			addListener(action);

			clearFuture();
			actions.push_back(std::move(action));
			redo();
		}

		/// Throws away everything after current position
		void clearFuture () {
			const size_t size = actions.size();
			for (size_t i = position; i < size; i++) {
				actions.pop_back();
			}
			clearFutureListener();
		}

		bool canUndo () const {
			return position > 0 && actions.at(position - 1)->canUndo();
		}

		bool canRedo () const {
			return position < actions.size();
		}

		/// Undoes action
		/// Action will be undone *after* position is modified.
		void undo () {
			if (!canUndo()) {
				return;
			}

			position--;
			auto& action = actions.at(position);
			action->undo();
		}

		/// Redoes action
		/// Action will be redone *after* position is modified
		void redo () {
			if (!canRedo()) {
				return;
			}

			position++;
			auto& action = actions.at(position - 1);
			action->redo();
		}

		template<typename F>
		void visit (F&& func) {
			detail::visitActions(func, actions);
		}

		template<typename F>
		void visitPast (F&& func) {
			for (size_t i = 0; i < position; i++) {
				detail::visitAction(func, actions);
			}
		}

		template<typename F>
		void visitFuture (F&& func) {
			for (size_t i = position; i < actions.size(); i++) {
				detail::visitAction(func, actions);
			}
		}
	};


	template<typename T>
	struct ActionListLink {
		using BaseType = T;

		struct LinkedAction : public Action {
			/// non-null
			ActionListLink<T>* link;
			size_t index;

			explicit LinkedAction (ActionListLink<T>* t_link, size_t t_index) : link(t_link), index(t_index) {}

			void undo () {
				link->sync();
			}

			void redo () {
				link->position = index + 1;
			}
		};

		ActionList& list;
		std::vector<std::unique_ptr<BaseType>> data;
		size_t position = 0;

		size_t clearFutureListenerID;
		explicit ActionListLink (ActionList& t_list) : list(t_list) {
			clearFutureListenerID = list.clearFutureListener.add([this] () {
				this->sync();
				this->clearFuture();
			});
		}

		void addAction (std::unique_ptr<BaseType>&& value) {
			clearFuture();
			std::unique_ptr<LinkedAction> linked_action = std::make_unique<LinkedAction>(
				this,
				data.size()
			);
			// NOTE: currently this will probably perform a wasteful sync
			list.add(std::move(linked_action));
			data.push_back(std::move(value));
			position = data.size();
		}

		void clearFuture () {
			const size_t size = data.size();
			for (size_t i = position; i < size; i++) {
				data.pop_back();
			}
		}

		void sync () {
			for (size_t i = list.position; i--;) {
				if (LinkedAction* linked_action = dynamic_cast<LinkedAction*>(list.actions.at(i).get())) {
					if (linked_action->link == this) {
						position = linked_action->index + 1;
					}
				}
			}
		}

		template<typename F>
		void visit (F&& func) {
			for (auto& value : data) {
				func(value);
			}
		}

		template<typename F>
		void visitPast (F&& func) {
			for (size_t i = 0; i < position; i++) {
				func(data.at(i));
			}
		}

		template<typename F>
		void visitFuture (F&& func) {
			for (size_t i = position; i < data.size(); i++) {
				func(data.at(i));
			}
		}
	};
};