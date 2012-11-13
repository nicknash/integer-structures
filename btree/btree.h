//
// This code has been modified to add a small amount of 
// extra genericity. That is, to provide a mapping from
// a key type to a value type.
//
// A simple interface has also been added.
//
// --
// Nicholas Nash,
// Dept. of Computer Science
// Trinity College Dublin
/*
from: textelectric.net
   linux version (without the timings):  december 3rd, 2006
this file contains a class template for elements stored in a B-tree, and a
class for a B-tree node, which provides all the methods needed to search,
insert, or delete.  a sample "main" function is also provided to show
how to use the B-tree.
to understand the identifiers and comments, visualize the tree as having
its root node at the top of the diagram, its leaf nodes at the bottom of
the diagram, and each node as containing an array oriented horizontally,
with the smallest element on the left and the largest element on the right.
the zeroth element of a node contains only a subtree pointer, no key value
or payload.
a b-tree grows "upward", by splitting the root node when the node's
capacity
is exceeded.  conversely, the depth of the tree is always reduced by
merging
the last remaining element of the root node with the elements of its two
child nodes, so the tree contracts "from the top".
this code may be freely copied.
programmer: toucan@textelectric.net
algorithm and pseudo-code found in:
"fundamentals of data structures in pascal", by Horowitz and Sahni
there is a java applet on the web that draws a b-tree diagram and allows
the
user to perform insertions and deletions, so you can see how it grows
and shrinks,
at: http://www.mars.dti.ne.jp/~torao/program/structure/btree.html
*/
#ifndef __BTREE_H

#define __BTREE_H

#include <iostream>
#include <vector>

using namespace std;

#define null_ptr 0

//class Node;
//Node* null_ptr = reinterpret_cast<Node*> (0);

const int invalid_index = -1;
const int max_elements = 200;  // max elements in a node
// size limit for the array in a vector object.  best performance was
// at 800 bytes.

const int max_array_bytes = 800;

template <class Elem> class Node;

template<class key, class payload> class Element {
// contains a key value, a payload, and a pointer toward the subtree
// containing key values greater than this->m_key but lower than the
// key value of the next element to the right
    static Node<Element>* invalid_ptr;
//    static Node<Element>* null_ptr;
public:
	key m_key;
	payload m_payload;
	Node<Element>* mp_subtree;
public:
	bool operator>   (Element& other) const { return m_key >  other.m_key; }
	bool operator<   (Element& other) const { return m_key <  other.m_key; }
	bool operator>=  (Element& other) const { return m_key >= other.m_key; }
	bool operator<=  (Element& other) const { return m_key <= other.m_key; }
	bool operator==  (Element& other) const { return m_key == other.m_key; }
	bool valid () const { return mp_subtree != invalid_ptr; }
	void invalidate () { mp_subtree = invalid_ptr; }
	Element& operator= (const Element& other) {
		m_key = other.m_key;
		m_payload = other.m_payload;
		mp_subtree = other.mp_subtree;
		return *this;
	}
	Element () { mp_subtree = null_ptr; }
}; //______________________________________________________________________

// Note from Nicholas: I apologize to humanity for this line of code.
template <class key, class payload> Node<Element<key, payload> >* Element<key, payload>::invalid_ptr = reinterpret_cast<Node<Element>*>(0xFFFFFFFF);

template <class Elem> class RootTracker;

template <class Elem> class Node {
protected:
    // locality of reference, beneficial to effective cache utilization,
    // is provided by a "vector" container rather than a "list"
	vector<Elem> m_vector;
    // number of elements currently in m_vector, including the zeroth element
    // which has only a subtree, no key value or payload.
	int m_count;
	Node<Elem>* mp_parent;
	bool is_leaf();
	bool vector_insert (Elem& element);
	bool vector_insert_for_split (Elem& element);
	bool split_insert (Elem& element);
	bool vector_delete (Elem& target);
	bool vector_delete (int target_pos);
	void insert_zeroth_subtree (Node<Elem>* subtree);
	void set_debug();
	int key_count () { return m_count-1; }
	Elem& largest_key () { return m_vector[m_count-1]; }
	Elem& smallest_key () { return m_vector[1]; }
	Elem& smallest_key_in_subtree();
	int index_has_subtree ();
	Node<Elem>* right_sibling (int& parent_index_this);
	Node<Elem>* left_sibling (int& parent_index_this);
	Node<Elem>* rotate_from_left(int parent_index_this);
	Node<Elem>* rotate_from_right(int parent_index_this);
	Node<Elem>* merge_right (int parent_index_this);
	Node<Elem>* merge_left (int parent_index_this);
	bool merge_into_root ();
	int minimum_keys ();

public:
	Elem& search (Elem& desired, Node<Elem>*& last_visited);
	bool tree_insert (Elem& element);
	bool delete_element (Elem& target);
	int delete_all_subtrees ();
	Node<Elem>* find_root();
    // to return a reference when a search fails.
	static Elem m_failure;
    // the root of the tree may change.  this attribute keeps it accessible.
	RootTracker<Elem>& m_root;
	Elem& operator[] (int i) { return m_vector[i]; }
    // node cannot be instantiated without a root tracker
	Node<Elem> (RootTracker<Elem>& root_track);
}; 

template <class Elem> Elem Node<Elem>::m_failure;// = Elem(0, 0);

//______________________________________________________________________
template <class Elem> class RootTracker {
// all the node instances that belong to a given tree have a reference to one
// instance of RootTracker.  as the Node instance that is the root may change
// or the original root may be deleted, Node instances must access the root
// of the tree through this tracker, and update the root pointer when they
// perform insertions or deletions.  using a static attribute in the Node
// class to hold the root pointer would limit a program to just one B-tree.
protected:
	Node<Elem>* mp_root;
public:
	RootTracker() { mp_root = null_ptr; }
	void set_root (Node<Elem>* old_root, Node<Elem>* new_root) {
			mp_root = new_root;
	}
	Node<Elem>* get_root () { return mp_root; }
	~RootTracker () {
        // safety measure
		if (mp_root) {
			mp_root->delete_all_subtrees();
			delete mp_root;
		}
	}
}; //_______________________________________________________________________

template <class Elem> int Node<Elem>::minimum_keys () {
    // minus 1 for the empty slot left for splitting the node
	int size = m_vector.size();
	int ceiling_func = (size-1)/2;
	if (ceiling_func*2 < size-1)
		ceiling_func++;
	return ceiling_func-1;  // for clarity, may increment then decrement
} //________________________________________________________________________

template <class Elem> void Node<Elem>::insert_zeroth_subtree (Node<Elem>* subtree) {
	m_vector[0].mp_subtree = subtree;
	m_vector[0].m_key = 0;
	m_count = 1;
	if (subtree)
		subtree->mp_parent = this;
}
//_________________________________________________________________________
template <class Elem> Node<Elem>::Node (RootTracker<Elem>& root_track)  : m_root(root_track) {
// limit the size of the vector to 4 kilobytes max and 200 entries max.
	int num_elements = max_elements*sizeof(Elem)<=(unsigned int) max_array_bytes ? max_elements : max_array_bytes/sizeof(Elem);
	if (num_elements < 6)  // in case key or payload is really huge
		num_elements = 6;
	m_vector.resize (num_elements);
	m_count = 0;
	mp_parent = 0;
	insert_zeroth_subtree (0);
} //________________________________________________________________________

template <class Elem> Node<Elem>* Node<Elem>::find_root () {
	Node<Elem>* current = this;
	while (current->mp_parent)
		current = current->mp_parent;
	return current;
}
//__________________________________________________________________________
template <class Elem> bool Node<Elem>::is_leaf () {
	return m_vector[0].mp_subtree==0;
} //________________________________________________________________________
template <class Elem> int Node<Elem>::delete_all_subtrees () {
// return the number of nodes deleted
	int count_deleted = 0;
	for (int i=0; i< m_count; i++) {
		if (!m_vector[i].mp_subtree)
			continue;
		else if (m_vector[i].mp_subtree->is_leaf()) {
			delete m_vector[i].mp_subtree;
			count_deleted++;
		}
		else
			count_deleted += m_vector[i].mp_subtree->delete_all_subtrees();
	}
	return count_deleted;
} //_______________________________________________________________________
template <class Elem> bool Node<Elem>::vector_insert (Elem& element) {
// this method merely tries to insert the argument into the current node.
// it does not concern itself with the entire tree.
// if the element can fit into m_vector, slide all the elements
// greater than the argument forward one position and copy the argument
// into the newly vacated slot, then return true.  otherwise return false.
// note: the tree_insert method will already have verified that the key
// value of the argument element is not present in the tree.
	if ((unsigned int) m_count >= m_vector.size()-1) // leave an extra slot for split_insert
		return false;
	int i = m_count;
	while (i>0 && m_vector[i-1]>element) {
		m_vector[i] = m_vector[i-1];
		i--;
	}
	if (element.mp_subtree)
		element.mp_subtree->mp_parent = this;
	m_vector[i] = element;
	m_count++;
	return true;
} //__________________________________________________________________
template <class Elem> bool Node<Elem>::vector_delete (Elem& target) {
// delete a single element in the vector belonging to *this node.
// if the target is not found, do not look in subtrees, just return false.
	int target_pos = -1;
	int first = 1;
	int last = m_count-1;
    // perform binary search
	while (last-first > 1) {
		int mid = first+(last-first)/2;
		if (target>=m_vector[mid])
			first = mid;
		else
			last = mid;
	}
	if (m_vector[first]==target)
		target_pos = first;
	else if (m_vector[last]==target)
		target_pos = last;
	else
		return false;
    // the element's subtree, if it exists, is to be deleted or re-attached
    // in a different function.  not a concern here.  just shift all the
    // elements in positions greater than target_pos.
	for (int i=target_pos; i<m_count; i++)
		m_vector[i] = m_vector[i+1];
	m_count--;
	return true;
} //____________________________________________________________________
template <class Elem> bool Node<Elem>::vector_delete (int target_pos) {
// delete a single element in the vector belonging to *this node.
// the element is identified by position, not value.
	if (target_pos<0 || target_pos>=m_count)
		return false;
    // the element's subtree, if it exists, is to be deleted or re-attached
    // in a different function.  not a concern here.  just shift all the
    // elements in positions greater than target_pos.
	for (int i=target_pos; i<m_count; i++)
		m_vector[i] = m_vector[i+1];
	m_count--;
	return true;
} //____________________________________________________________________
template <class Elem> bool Node<Elem>::vector_insert_for_split (Elem& element) {
// this method insert an element that is in excess of the nominal capacity of
// the node, using the extra slot that always remains unused during normal
// insertions.  this method should only be called from split_insert()
	if ((unsigned int)m_count >= m_vector.size()) // error
		return false;
	int i = m_count;
	while (i>0 && m_vector[i-1]>element) {
		m_vector[i] = m_vector[i-1];
		i--;
	}
	if (element.mp_subtree)
		element.mp_subtree->mp_parent = this;
	m_vector[i] = element;
	m_count++;
	return true;
} //__________________________________________________________________
template <class Elem> bool Node<Elem>::split_insert (Elem& element) {
    // split_insert should only be called if node is full
	vector_insert_for_split (element);
	int split_point = m_count/2;
	if (2*split_point < m_count)  // perform the "ceiling function"
		split_point++;
    // new node receives the rightmost half of elements in *this node
	Node<Elem>* new_node = new Node<Elem>(m_root);
	Elem upward_element = m_vector[split_point];
	new_node->insert_zeroth_subtree (upward_element.mp_subtree);
	upward_element.mp_subtree = new_node;
    // element that gets added to the parent of this node
	for (int i=1; i<m_count-split_point; i++)
		new_node->vector_insert(m_vector[split_point+i]);
	new_node->m_count = m_count-split_point;
	m_count = split_point;
	new_node->mp_parent = mp_parent;
    // now insert the new node into the parent, splitting it if necessary
	if (mp_parent && mp_parent->vector_insert(upward_element))
		return true;
	else if (mp_parent && mp_parent->split_insert(upward_element))
		return true;
	else if (!mp_parent) { // this node was the root
		Node<Elem>* new_root = new Node<Elem>(m_root);
		new_root->insert_zeroth_subtree(this);
		this->mp_parent = new_root;
		new_node->mp_parent = new_root;
		new_root->vector_insert (upward_element);
		m_root.set_root (m_root.get_root(),  new_root);
		new_root->mp_parent = 0;
	}
	return true;
}//__________________________________________________________________
template <class Elem> bool Node<Elem>::tree_insert (Elem& element) {
	Node<Elem>* last_visited_ptr = this;
	if (search(element, last_visited_ptr).valid())  // element already in tree
		return false;
	if (last_visited_ptr->vector_insert(element))
		return true;
	return last_visited_ptr->split_insert(element);
} //__________________________________________________________________
template <class Elem> bool Node<Elem>::delete_element (Elem& target) {
// target is just a package for the key value.  the reference does not
// provide the address of the Elem instance to be deleted.
// first find the node contain the Elem instance with the given key
	Node<Elem>* node = 0;
	int parent_index_this = invalid_index;
	Elem& found = search (target, node);
	if (!found.valid())
		return false;
	if (node->is_leaf() && node->key_count() > node->minimum_keys())
		return node->vector_delete (target);
	else if (node->is_leaf()) {
		node->vector_delete (target);
        // loop invariant: if _node_ is not null_ptr, it points to a node
        // that has lost an element and needs to import one from a sibling
        // or merge with a sibling and import one from its parent.
        // after an iteration of the loop, _node_ may become null or
        // it may point to its parent if an element was imported from the
        // parent and this caused the parent to fall below the minimum
        // element count.
		while (node) {
            // NOTE: the "this" pointer may no longer be valid after the first
            // iteration of this loop!!!
			if (node==node->find_root() && node->is_leaf())
				break;
            // is an extra element available from the right sibling (if any)
			Node<Elem>* right = node->right_sibling(parent_index_this);
			if (right && right->key_count() > right->minimum_keys())
				node = node->rotate_from_right(parent_index_this);
			else {
                // is an extra element available from the left sibling (if any)
				Node<Elem>* left = node->left_sibling(parent_index_this);
				if (left && left->key_count() > left->minimum_keys())
					node = node->rotate_from_left(parent_index_this);
				else if (right)
					node = node->merge_right(parent_index_this);
				else if (left)
					node = node->merge_left(parent_index_this);
			}
		}
	}
	else {
		Elem& smallest_in_subtree =
			found.mp_subtree->smallest_key_in_subtree();
		found.m_key = smallest_in_subtree.m_key;
		found.m_payload = smallest_in_subtree.m_payload;
		found.mp_subtree->delete_element (smallest_in_subtree);
	}
	return true;
} //___________________________________________________________________
template <class Elem> Node<Elem>* Node<Elem>::rotate_from_right(int parent_index_this) {
    // new element to be added to this node
	Elem underflow_filler = (*mp_parent)[parent_index_this+1];
    // right sibling of this node
	Node<Elem>* right_sib = (*mp_parent)[parent_index_this+1].mp_subtree;
	underflow_filler.mp_subtree = (*right_sib)[0].mp_subtree;
    // copy the entire element
	(*mp_parent)[parent_index_this+1] = (*right_sib)[1];
    // now restore correct pointer
	(*mp_parent)[parent_index_this+1].mp_subtree = right_sib;
	vector_insert (underflow_filler);
	right_sib->vector_delete(0);
	(*right_sib)[0].m_key = 0;
	(*right_sib)[0].m_payload = 0;
	return null_ptr; // parent node still has same element count
} //_______________________________________________________________________
template <class Elem> Node<Elem>* Node<Elem>::rotate_from_left(int parent_index_this) {
    // new element to be added to this node
	Elem underflow_filler = (*mp_parent)[parent_index_this];
    // left sibling of this node
	Node<Elem>* left_sib = (*mp_parent)[parent_index_this-1].mp_subtree;
	underflow_filler.mp_subtree = (*this)[0].mp_subtree;
	(*this)[0].mp_subtree = (*left_sib)[left_sib->m_count-1].mp_subtree;
	if ((*this)[0].mp_subtree)
		(*this)[0].mp_subtree->mp_parent = this;
    // copy the entire element
	(*mp_parent)[parent_index_this] = (*left_sib)[left_sib->m_count-1];
    // now restore correct pointer
	(*mp_parent)[parent_index_this].mp_subtree = this;
	vector_insert (underflow_filler);
	left_sib->vector_delete(left_sib->m_count-1);
	return null_ptr; // parent node still has same element count
} //_______________________________________________________________________
template <class Elem> Node<Elem>* Node<Elem>::merge_right (int parent_index_this) {
// copy elements from the right sibling into this node, along with the
// element in the parent node vector that has the right sibling as it subtree.
// the right sibling and that parent element are then deleted
	Elem parent_elem = (*mp_parent)[parent_index_this+1];
	Node<Elem>* right_sib = (*mp_parent)[parent_index_this+1].mp_subtree;
	parent_elem.mp_subtree = (*right_sib)[0].mp_subtree;
	vector_insert (parent_elem);
	for (int i=1; i<right_sib->m_count; i++)
		vector_insert ((*right_sib)[i]);
	mp_parent->vector_delete (parent_index_this+1);
	delete right_sib;
	if (mp_parent==find_root() && !mp_parent->key_count()) {
		m_root.set_root(m_root.get_root(), this);
		delete mp_parent;
		mp_parent = 0;
		return null_ptr;
	}
	else if (mp_parent==find_root() && mp_parent->key_count())
		return null_ptr;
	if (mp_parent&& mp_parent->key_count() >= mp_parent->minimum_keys())
		return null_ptr; // no need for parent to import an element
	return mp_parent; // parent must import an element
} //_______________________________________________________________________
template <class Elem> Node<Elem>* Node<Elem>::merge_left (int parent_index_this) {
// copy all elements from this node into the left sibling, along with the
// element in the parent node vector that has this node as its subtree.
// this node and its parent element are then deleted.
	Elem parent_elem = (*mp_parent)[parent_index_this];
	parent_elem.mp_subtree = (*this)[0].mp_subtree;
	Node<Elem>* left_sib = (*mp_parent)[parent_index_this-1].mp_subtree;
	left_sib->vector_insert (parent_elem);
	for (int i=1; i<m_count; i++)
		left_sib->vector_insert (m_vector[i]);
	mp_parent->vector_delete (parent_index_this);
	Node<Elem>* parent_node = mp_parent;  // copy before deleting this node
	if (mp_parent==find_root() && !mp_parent->key_count()) {
		m_root.set_root(m_root.get_root(), left_sib);
		delete mp_parent;
		left_sib->mp_parent = null_ptr;
		delete this;
		return null_ptr;
	}
	else if (mp_parent==find_root() && mp_parent->key_count()) {
		delete this;
		return null_ptr;
	}
	delete this;
	if (parent_node->key_count() >= parent_node->minimum_keys())
		return null_ptr; // no need for parent to import an element
	return parent_node; // parent must import an element
} //_______________________________________________________________________
template <class Elem> Node<Elem>* Node<Elem>::right_sibling (int& parent_index_this) {
	parent_index_this = index_has_subtree (); // for element with THIS as subtree
	if (parent_index_this == invalid_index)
		return 0;
    // now mp_parent is known not to be null
	if (parent_index_this >= mp_parent->m_count-1)
		return 0;  // no right sibling
	return mp_parent->m_vector[parent_index_this+1].mp_subtree;  // might be null
}
//__________________________________________________________________________
template <class Elem> Node<Elem>* Node<Elem>::left_sibling (int& parent_index_this) {
	parent_index_this = index_has_subtree (); // for element with THIS as subtree
	if (parent_index_this == invalid_index)
		return 0;
    // now mp_parent is known not to be null
	if (parent_index_this==0)
		return 0;  // no left sibling
	return mp_parent->m_vector[parent_index_this-1].mp_subtree;  // might be null
}
//____________________________________________________________________________
template <class Elem> int Node<Elem>::index_has_subtree () {
// return the element in this node's parent that has the "this" pointer as its subtree
	if (!mp_parent)
		return invalid_index;
	int first = 0;
	int last = mp_parent->m_count-1;
	while (last-first > 1) {
		int mid = first+(last-first)/2;
		//Elem& smallest = smallest_key();
		if (smallest_key()>=mp_parent->m_vector[mid])
			first = mid;
		else
			last = mid;
	}
	if (mp_parent->m_vector[first].mp_subtree == this)
		return first;
	else if (mp_parent->m_vector[last].mp_subtree == this)
		return last;

    return invalid_index;
} //__________________________________________________________________
template <class Elem> Elem& Node<Elem>::smallest_key_in_subtree () {
	if (is_leaf())
		return m_vector[1];
	else
		return m_vector[0].mp_subtree->smallest_key_in_subtree();
} //___________________________________________________________________
template <class Elem> Elem& Node<Elem>::search (Elem& desired, Node<Elem>*& last_visited_ptr) {
    // the zeroth element of the vector is a special case (no key value or
    // payload, just a subtree).  the seach starts at the *this node, not
    // at the root of the b-tree.
	Node<Elem>* current = this;
	if (!key_count())
		current = 0;
	while (current) {
		last_visited_ptr = current;
        // if desired is less than all values in current node
		if (current->m_count>1 && desired<current->m_vector[1])
			current = current->m_vector[0].mp_subtree;
        // if desired is greater than all values in current node
		else if (desired>current->m_vector[current->m_count-1])
			current = current->m_vector[current->m_count-1].mp_subtree;
		else {
            // binary search of the node
			int first = 1;
			int last = current->m_count-1;
			while (last-first > 1) {
				int mid = first+(last-first)/2;
				if (desired>=current->m_vector[mid])
					first = mid;
				else
					last = mid;
			}
			if (current->m_vector[first]==desired)
				return current->m_vector[first];
			if (current->m_vector[last]==desired)
				return current->m_vector[last];
			else if (current->m_vector[last]>desired)
				current = current->m_vector[first].mp_subtree;
			else
				current = current->m_vector[last].mp_subtree;
		}
	}
	return m_failure;
} //_____________________________________________________________________


template <class KeyType, class ValueType> class BTree
{
    typedef Element<KeyType, ValueType> Elem;
    Elem elem, result;   
    Node<Elem>* last;
    RootTracker<Elem> tracker;
public:
    BTree()
    {
        Node<Elem>::m_failure.invalidate();
        Node<Elem>::m_failure.m_key = 0;
        
        Node<Elem>* root_ptr = new Node<Elem>(tracker);
        tracker.set_root(null_ptr, root_ptr);
        return;
    }
    void insert(const KeyType& key, const ValueType& value)
    {        
        elem.m_key = key;
        elem.m_payload = value;
        tracker.get_root()->tree_insert(elem);
        return;
    }
    ValueType* search(const KeyType& key)
    {
        elem.m_key = key; 
        result = tracker.get_root()->search(elem, last);
        return &(result.m_payload);
    }

    ValueType* locate(const KeyType& key) 
    {
        // This should actually locate, and not just search.
        // However not doing the locate only biases the results *against*
        // my own data structures, i.e. making them look (perhaps immesurably)
        // worse than they really are.
        elem.m_key = key; 
        result = tracker.get_root()->search(elem, last);
        return &(result.m_payload);
    }
    void remove(const KeyType& key)
    {
        elem.m_key = key;
        tracker.get_root()->delete_element(elem);
        return;
    }
    ~BTree()
    {
        return;
    }
};


#endif
