#include <include/MlActions.hpp>
#include <cassert>

using namespace MlActions;

static int latest_redo = 0;
static int latest_undo = 0;
struct test_Action1 : public Action {
	int x;

	test_Action1 (int t_x) : x(t_x) {}

	void redo () override {
		latest_redo = x;
	}

	void undo () override {
		latest_undo = x;
	}
};
struct Base {
	Base () {}
	virtual ~Base () {}
};
struct Alpha : public Base {
	size_t value;
	explicit Alpha (size_t t_value) : value(t_value) {}
};
struct Beta : public Base {
	size_t position;
	explicit Beta (size_t t_position) : position(t_position) {}
};
static void test () {
	ActionList list;

	assert(latest_redo == 0);
	assert(latest_undo == 0);
	assert(list.position == 0);
	assert(list.actions.size() == 0);
	assert(list.canUndo() == false);
	assert(list.canRedo() == false);

	// Begin simple test - Adding elements
	list.add(std::make_unique<test_Action1>(1));
	assert(list.position == 1);
	assert(list.actions.size() == 1);
	assert(latest_redo == 1);
	assert(latest_undo == 0);
	assert(list.canUndo() == true);
	assert(list.canRedo() == false);

	list.add(std::make_unique<test_Action1>(2));
	assert(list.position == 2);
	assert(list.actions.size() == 2);
	assert(latest_redo == 2);
	assert(latest_undo == 0);
	assert(list.canUndo() == true);
	assert(list.canRedo() == false);

	list.add(std::make_unique<test_Action1>(3));
	assert(list.position == 3);
	assert(list.actions.size() == 3);
	assert(latest_redo == 3);
	assert(latest_undo == 0);
	assert(list.canUndo() == true);
	assert(list.canRedo() == false);

	// Begin simple undo test
	list.undo();
	assert(list.position == 2);
	assert(list.actions.size() == 3);
	assert(latest_redo == 3);
	assert(latest_undo == 3);
	assert(list.canUndo() == true);
	assert(list.canRedo() == true);

	list.undo();
	assert(list.position == 1);
	assert(list.actions.size() == 3);
	assert(latest_redo == 3);
	assert(latest_undo == 2);
	assert(list.canUndo() == true);
	assert(list.canRedo() == true);

	list.undo();
	assert(list.position == 0);
	assert(list.actions.size() == 3);
	assert(latest_redo == 3);
	assert(latest_undo == 1);
	assert(list.canUndo() == false);
	assert(list.canRedo() == true);

	// Begin simple redo test
	list.redo();
	assert(list.position == 1);
	assert(list.actions.size() == 3);
	assert(latest_redo == 1);
	assert(latest_undo == 1);
	assert(list.canUndo() == true);
	assert(list.canRedo() == true);

	list.redo();
	assert(list.position == 2);
	assert(list.actions.size() == 3);
	assert(latest_redo == 2);
	assert(latest_undo == 1);
	assert(list.canUndo() == true);
	assert(list.canRedo() == true);

	list.redo();
	assert(list.position == 3);
	assert(list.actions.size() == 3);
	assert(latest_redo == 3);
	assert(latest_undo == 1);
	assert(list.canUndo() == true);
	assert(list.canRedo() == false);

	// Begin mixed

	list.undo();
	assert(list.position == 2);
	assert(list.actions.size() == 3);
	assert(latest_redo == 3);
	assert(latest_undo == 3);
	assert(list.canUndo() == true);
	assert(list.canRedo() == true);

	list.redo();
	assert(list.position == 3);
	assert(list.actions.size() == 3);
	assert(latest_redo == 3);
	assert(latest_undo == 3);
	assert(list.canUndo() == true);
	assert(list.canRedo() == false);



	// Begin undo add instances

	list.undo();
	list.undo();

	list.add(std::make_unique<test_Action1>(4));
	assert(list.position == 2);
	assert(list.actions.size() == 2);
	assert(latest_redo == 4);
	assert(latest_undo == 2);
	assert(list.canUndo() == true);
	assert(list.canRedo() == false);

	list.undo();

	// Begin ActionListLink

	ActionListLink<Base> link(list);

	assert(link.data.size() == 0);
	assert(link.position == 0);

	link.addAction(std::make_unique<Alpha>(42));
	assert(link.data.size() == 1);
	assert(link.position == 1);
	assert(list.actions.size() == 2);
	assert(list.position == 2);
	assert(dynamic_cast<ActionListLink<Base>::LinkedAction*>(list.actions.at(1).get()) != nullptr);
	assert(dynamic_cast<ActionListLink<Base>::LinkedAction*>(list.actions.at(1).get())->index == 0);
	assert(dynamic_cast<Alpha*>(link.data.at(0).get()) != nullptr);
	assert(dynamic_cast<Alpha*>(link.data.at(0).get())->value == 42);

	link.addAction(std::make_unique<Beta>(64));
	assert(link.data.size() == 2);
	assert(link.position == 2);
	assert(list.actions.size() == 3);
	assert(list.position == 3);
	assert(dynamic_cast<ActionListLink<Base>::LinkedAction*>(list.actions.at(2).get()) != nullptr);
	assert(dynamic_cast<ActionListLink<Base>::LinkedAction*>(list.actions.at(2).get())->index == 1);
	assert(dynamic_cast<Beta*>(link.data.at(1).get()) != nullptr);
	assert(dynamic_cast<Beta*>(link.data.at(1).get())->position == 64);

	list.undo();

	assert(link.data.size() == 2);
	assert(link.position == 1);
	assert(list.actions.size() == 3);
	assert(list.position == 2);
	assert(dynamic_cast<ActionListLink<Base>::LinkedAction*>(list.actions.at(1).get()) != nullptr);
	assert(dynamic_cast<ActionListLink<Base>::LinkedAction*>(list.actions.at(1).get())->index == 0);
	assert(dynamic_cast<Alpha*>(link.data.at(0).get()) != nullptr);
	assert(dynamic_cast<Alpha*>(link.data.at(0).get())->value == 42);
	assert(dynamic_cast<ActionListLink<Base>::LinkedAction*>(list.actions.at(2).get()) != nullptr);
	assert(dynamic_cast<ActionListLink<Base>::LinkedAction*>(list.actions.at(2).get())->index == 1);
	assert(dynamic_cast<Beta*>(link.data.at(1).get()) != nullptr);
	assert(dynamic_cast<Beta*>(link.data.at(1).get())->position == 64);
}

int main () {
	test();
	return 0;
}