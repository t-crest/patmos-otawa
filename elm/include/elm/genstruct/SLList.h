/*
 *	$Id$
 *	SLList class interface
 *
 *	This file is part of OTAWA
 *	Copyright (c) 2004-08, IRIT UPS.
 * 
 *	OTAWA is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	OTAWA is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with OTAWA; if not, write to the Free Software 
 *	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef ELM_GENSTRUCT_SLLIST_H
#define ELM_GENSTRUCT_SLLIST_H

#include <elm/assert.h>
#include <elm/PreIterator.h>
#include <elm/util/Equiv.h>
#include <elm/inhstruct/SLList.h>

namespace elm { namespace genstruct {

// SLList class
template <class T, class E = Equiv<T> >
class SLList {
	inhstruct::SLList list;
	
	// Node class
	class Node: public inhstruct::SLNode {
	public:
		T val;
		inline Node(const T value): val(value) { }
		inline Node *next(void) const { return static_cast<Node *>(SLNode::next()); }
	};

public:
	inline SLList(void) { }
	inline SLList(const SLList& list) { copy(list); }
	inline SLList& operator=(const SLList& list) { copy(list); return *this; }
	inline ~SLList(void) { clear(); }
	
	inline void copy(const SLList<T>& list) {
		clear(); if(list) {
			Iterator item(list); addFirst(*item);
			Node *cur = static_cast<Node *>(this->list.first());
			for(item++; item; item++) { cur->insertAfter(new Node(*item)); cur = cur->next(); }
		}
	}

	static SLList<T, E> null;

	// Collection concept
	inline int count(void) const { return list.count(); }
	inline bool contains (const T &item) const
		{ for(Iterator iter(*this); iter; iter++) if(E::equals(item, iter)) return true; return false; }
	inline bool isEmpty(void) const { return list.isEmpty(); };
	inline operator bool(void) const { return !isEmpty(); }

	// Iterator class
	class Iterator: public PreIterator<Iterator, const T&> {
	public:
		inline Iterator(void): node(0), prev(0) { }
		inline Iterator(const SLList& _list): node(static_cast<Node *>(_list.list.first())), prev(0) { }
		inline Iterator(const Iterator& source): node(source.node), prev(source.prev) { }
		inline Iterator& operator=(const Iterator& i) { node = i.node; prev = i.prev; return *this; }
		
		inline bool ended(void) const { return !node; }
		inline const T& item(void) const { ASSERT(node); return node->val; }
		inline void next(void)
			{ ASSERT(node); prev = node; node = node->next(); }
	private:
		friend class SLList;
		Node *node, *prev;
	};

	// MutableIterator class
	class MutableIterator: public PreIterator<Iterator, T&> {
	public:
		inline MutableIterator(void): node(0), prev(0) { }
		inline MutableIterator(const SLList& _list): node(static_cast<Node *>(_list.list.first())), prev(0) { }
		inline MutableIterator(const MutableIterator& source): node(source.node), prev(source.prev) { }
		inline MutableIterator& operator=(const MutableIterator& i) { node = i.node; prev = i.prev; return *this; }
		
		inline bool ended(void) const { return !node; }
		inline T& item(void) const { ASSERT(node); return node->val; }
		inline void next(void)
			{ ASSERT(node); prev = node; node = node->next(); }
	private:
		friend class SLList;
		Node *node, *prev;
	};

	// MutableCollection concept
	inline void clear(void)
		{ while(!list.isEmpty()) { Node *node = static_cast<Node *>(list.first()); list.removeFirst(); delete node; } }
	inline void add(const T& value) { addFirst(value); }
	template <class C> inline void addAll(const C& items)
		{ for(typename C::Iterator iter(items); iter; iter++) add(iter); }
	template <class C> inline void removeAll(const C& items)
		{ for(typename C::Iterator iter(items); iter; iter++) remove(iter);	}

	void remove(const T& value) {
		for(Node *prv = 0, *cur = static_cast<Node *>(list.first()); cur; prv = cur, cur = cur->next())
			if(E::equals(cur->val, value))
				{ if(!prv) list.removeFirst(); else prv->removeNext(); }
	}

	void remove(Iterator &iter) {
		ASSERT(iter.node);
		if(!iter.prev) removeFirst(); else iter.prev->removeNext();
		if(iter.prev) iter.node = iter.prev->next(); else iter.node = static_cast<Node *>(list.first());
	}
	
	// List concept
	inline T& first(void) { return static_cast<Node *>(list.first())->val; }
	inline const T& first(void) const { return static_cast<Node *>(list.first())->val; }
	inline T& last(void) { return (static_cast<Node *>(list.last()))->val; }
	inline const T& last(void) const { return (static_cast<Node *>(list.last()))->val; }
	Iterator find(const T& item) const
		{ Iterator iter(*this); for(; iter; iter++) if(E::equals(item, iter)) break; return iter;  }
	Iterator find(const T& item, const Iterator& pos) const
		{ Iterator iter(pos); for(iter++; iter; iter++) if(E::equals(item, iter)) break; return iter; }
	
	// MutableList concept
	inline void addFirst(const T& value) { list.addFirst(new Node(value)); }
	inline void addLast(const T& value) { list.addLast(new Node(value)); }
	inline void addAfter(const Iterator& pos, const T& value)
		{ ASSERT(pos.node); pos.node->insertAfter(new Node(value)); }
	inline void addBefore(const Iterator& pos, const T& value)
		{ if(!pos.prev) addFirst(value); else pos.prev->insertAfter(new Node(value)); }
	inline void removeFirst(void)
		{ Node *node = static_cast<Node *>(list.first()); list.removeFirst(); delete node; }
	inline void removeLast(void)
		{ Node *node = static_cast<Node *>(list.last()); list.removeLast(); delete node; }
	inline void set(const Iterator &pos, const T &item) { ASSERT(pos.node); pos.node->val = item; }

	// operators
	inline SLList<T>& operator+=(const T& h) { addFirst(h); return *this; }
	inline SLList<T>& operator+=(const SLList<T>& l) { addAll(l); return *this; }
};

template <class T, class E> SLList<T, E> SLList<T, E>::null;

} } // elm::genstruct

#endif // ELM_GENSTRUCT_SLLIST_H
