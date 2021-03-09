/*!
  @file   vertex.cpp
  @brief  Implementation of vertex.H
  @author Robert Marskar
  @date   March 2021
*/

#include "dcel_vertex.H"
#include "dcel_edge.H"
#include "dcel_face.H"
#include "dcel_iterator.H"

using namespace dcel;

vertex::vertex(){
  m_pos    = RealVect::Zero;
  m_normal = RealVect::Zero;

  m_faceCache.resize(0);
}

vertex::vertex(const RealVect& a_pos){
  m_pos    = a_pos;
  m_normal = RealVect::Zero;
}

vertex::vertex(const RealVect& a_pos, const RealVect& a_normal){
  m_pos    = a_pos;
  m_normal = a_normal;
}

vertex::vertex(const vertex& a_otherVertex){
  this->define(a_otherVertex.getPosition(),
	       a_otherVertex.getEdge(),
	       a_otherVertex.getNormal());
}

vertex::~vertex(){

}

void vertex::define(const RealVect& a_pos, const std::shared_ptr<edge>& a_edge, const RealVect a_normal) noexcept {
  this->setPosition(a_pos);
  this->setEdge(a_edge);
  this->setNormal(a_normal);
}

void vertex::setPosition(const RealVect& a_pos) noexcept {
  m_pos = a_pos;
}

void vertex::setEdge(const std::shared_ptr<edge>& a_edge) noexcept {
  m_edge = a_edge;
}

void vertex::setNormal(const RealVect& a_normal) noexcept {
  m_normal = a_normal;
}

void vertex::addFaceToCache(const std::shared_ptr<face>& a_face) noexcept {
  m_faceCache.push_back(a_face);
}

void vertex::clearFaceCache() noexcept {
  m_faceCache.resize(0);
}

void vertex::normalizeNormalVector() noexcept {
  m_normal = m_normal/m_normal.vectorLength();
}

RealVect& vertex::getPosition() noexcept {
  return m_pos;
}

const RealVect& vertex::getPosition() const noexcept {
  return m_pos;
}

std::shared_ptr<edge>& vertex::getEdge() noexcept {
  return m_edge;
}

const std::shared_ptr<edge>& vertex::getEdge() const noexcept {
  return m_edge;
}

RealVect& vertex::getNormal() noexcept {
  return m_normal;
}

const RealVect& vertex::getNormal() const noexcept {
  return m_normal;
}

std::vector<std::shared_ptr<face> > vertex::getFaces() noexcept {
  std::vector<std::shared_ptr<face> > faces;
  for (edge_iterator iter(*this); iter.ok(); ++iter){
    faces.push_back(iter()->getFace());
  }

  return faces;
}

const std::vector<std::shared_ptr<face> >& vertex::getFaceCache() const noexcept{
  return m_faceCache;
}

std::vector<std::shared_ptr<face> >& vertex::getFaceCache() noexcept {
  return m_faceCache;
}

Real vertex::signedDistance(const RealVect& a_x0) const noexcept {
  const RealVect delta = a_x0 - m_pos;
  const Real dist      = delta.vectorLength();
  const Real dot       = m_normal.dotProduct(delta);
  const int sign       = (dot > 0.) ? 1 : -1;
  
  return dist*sign;
}

Real vertex::unsignedDistance2(const RealVect& a_x0) const noexcept {
  const RealVect d = a_x0 - m_pos;

  return d.dotProduct(d);
}
