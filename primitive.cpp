#include <algorithm>
#include "primitive.hpp"

Primitive::Primitive(int x)
{
	m_data = x;
	m_parent_attribute = NULL;
}

Primitive::Primitive(const Primitive & other)
{
	m_data = other.m_data;
	m_parent_attribute = other.m_parent_attribute;
}

Primitive& Primitive::operator=(const Primitive & other)
{
	Primitive tmp(other);
	swap(tmp);
	return *this;
}

void Primitive::swap(Primitive & other)
{
	std::swap(m_data, other.m_data);
}

Primitive::~Primitive()
{
}

void Primitive::accept(Visitor *v)
{ 
	v->visitPrimitive(this); 
}

Primitive* Primitive::clone() const
{
	return new Primitive(*this);
}


