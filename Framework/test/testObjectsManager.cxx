// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   Publisher_test.cpp
/// \author Barthelemy von Haller
///

#include "QualityControl/ObjectsManager.h"

#define BOOST_TEST_MODULE ObjectManager test
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <TObjString.h>
#include <TObjArray.h>
#include <TH1F.h>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace o2::quality_control::core;
using namespace AliceO2::Common;

namespace o2::quality_control::core
{

struct Config {
  std::string taskName = "test";
  std::string detectorName = "TST";
  std::string consulUrl = "invalid";
  std::string taskClass = "TestClass";
};

BOOST_AUTO_TEST_CASE(invalid_url_test)
{
  Config config;
  config.taskName = "test";
  config.consulUrl = "bad-url:1234";
  ObjectsManager objectsManager(config.taskName, config.taskClass, config.detectorName, 0);
}

BOOST_AUTO_TEST_CASE(duplicate_object_test)
{
  Config config;
  config.taskName = "test";
  config.consulUrl = "";
  ObjectsManager objectsManager(config.taskName, config.taskClass, config.detectorName, 0);
  TObjString s("content");
  objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever);
  BOOST_CHECK_NO_THROW(objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever));
  BOOST_REQUIRE(objectsManager.getMonitorObject("content") != nullptr);

  TObjString s2("content");
  BOOST_CHECK_NO_THROW(objectsManager.startPublishing<true>(&s2, PublicationPolicy::Forever));
  auto mo2 = objectsManager.getMonitorObject("content");
  BOOST_REQUIRE(mo2 != nullptr);
  BOOST_REQUIRE(mo2->getObject() != &s);
  BOOST_REQUIRE(mo2->getObject() == &s2);
}

BOOST_AUTO_TEST_CASE(is_being_published_test)
{
  Config config;
  config.taskName = "test";
  config.consulUrl = "";
  ObjectsManager objectsManager(config.taskName, config.taskClass, config.detectorName, 0);
  TObjString s("content");
  BOOST_CHECK(!objectsManager.isBeingPublished("content"));
  objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever);
  BOOST_CHECK_NO_THROW(objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever));
  BOOST_CHECK(objectsManager.isBeingPublished("content"));
}

BOOST_AUTO_TEST_CASE(unpublish_test)
{
  Config config;
  config.taskName = "test";
  ObjectsManager objectsManager(config.taskName, config.taskClass, config.detectorName, 0);
  TObjString s("content");
  objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever);
  BOOST_CHECK_EQUAL(objectsManager.getNumberPublishedObjects(), 1);
  objectsManager.stopPublishing(&s);
  BOOST_CHECK_EQUAL(objectsManager.getNumberPublishedObjects(), 0);
  objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever);
  BOOST_CHECK_EQUAL(objectsManager.getNumberPublishedObjects(), 1);
  objectsManager.stopPublishing("content");
  BOOST_CHECK_EQUAL(objectsManager.getNumberPublishedObjects(), 0);
  BOOST_CHECK_THROW(objectsManager.stopPublishing("content"), ObjectNotFoundError);
  BOOST_CHECK_THROW(objectsManager.stopPublishing("asdf"), ObjectNotFoundError);

  // unpublish all
  objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever);
  BOOST_CHECK_EQUAL(objectsManager.getNumberPublishedObjects(), 1);
  objectsManager.stopPublishingAll();
  BOOST_CHECK_EQUAL(objectsManager.getNumberPublishedObjects(), 0);
  BOOST_CHECK_NO_THROW(objectsManager.stopPublishingAll());

  // unpublish after deletion
  auto s2 = new TObjString("content");
  objectsManager.startPublishing<true>(s2, PublicationPolicy::Forever);
  BOOST_CHECK_EQUAL(objectsManager.getNumberPublishedObjects(), 1);
  delete s2;
  objectsManager.stopPublishing(s2);
  BOOST_CHECK_EQUAL(objectsManager.getNumberPublishedObjects(), 0);

  // unpublish for publication policy
  auto s3 = new TObjString("content3");
  objectsManager.startPublishing<true>(s3, PublicationPolicy::Once);
  auto s4 = new TObjString("content4");
  objectsManager.startPublishing<true>(s4, PublicationPolicy::Once);
  auto s5 = new TObjString("content5");
  objectsManager.startPublishing<true>(s5, PublicationPolicy::ThroughStop);
  BOOST_CHECK_EQUAL(objectsManager.getNumberPublishedObjects(), 3);
  objectsManager.stopPublishing(PublicationPolicy::Once);
  BOOST_CHECK_EQUAL(objectsManager.getNumberPublishedObjects(), 1);
  objectsManager.stopPublishing(PublicationPolicy::ThroughStop);
  BOOST_CHECK_EQUAL(objectsManager.getNumberPublishedObjects(), 0);

  objectsManager.startPublishing<true>(s3, PublicationPolicy::Once);
  objectsManager.stopPublishing(s3);
  BOOST_CHECK_EQUAL(objectsManager.getNumberPublishedObjects(), 0);
  BOOST_CHECK_NO_THROW(objectsManager.stopPublishing(PublicationPolicy::Once));
  BOOST_CHECK_EQUAL(objectsManager.getNumberPublishedObjects(), 0);
  BOOST_CHECK_NO_THROW(objectsManager.stopPublishing(s3));

  delete s3;
  delete s4;
  delete s5;
}

BOOST_AUTO_TEST_CASE(getters_test)
{
  Config config;
  config.taskName = "test";
  config.consulUrl = "";
  ObjectsManager objectsManager(config.taskName, config.taskClass, config.detectorName, 0);

  TObjString s("content");
  TH1F h("histo", "h", 100, 0, 99);

  objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever);
  objectsManager.startPublishing<true>(&h, PublicationPolicy::Forever);

  // basic gets
  BOOST_CHECK_NO_THROW(objectsManager.getMonitorObject("content"));
  BOOST_CHECK_NO_THROW(objectsManager.getMonitorObject("histo"));
  BOOST_CHECK_THROW(objectsManager.getMonitorObject("unexisting object"), ObjectNotFoundError);

  // non owning array
  TObjArray* array = objectsManager.getNonOwningArray();
  BOOST_CHECK_EQUAL(array->GetEntries(), 2);
  BOOST_CHECK(array->FindObject("content") != nullptr);
  BOOST_CHECK(array->FindObject("histo") != nullptr);

  // we confirm that deleting the array does not delete objects
  delete array;
  BOOST_CHECK_NO_THROW(objectsManager.getMonitorObject("content"));
  BOOST_CHECK_NO_THROW(objectsManager.getMonitorObject("histo"));
}

BOOST_AUTO_TEST_CASE(metadata_test)
{
  Config config;
  config.taskName = "test";
  config.consulUrl = "";
  ObjectsManager objectsManager(config.taskName, config.taskClass, config.detectorName, 0);

  TObjString s("content");
  TH1F h("histo", "h", 100, 0, 99);
  objectsManager.startPublishing<true>(&s, PublicationPolicy::Forever);
  objectsManager.startPublishing<true>(&h, PublicationPolicy::Forever);

  objectsManager.addMetadata("content", "aaa", "bbb");
  BOOST_CHECK_EQUAL(objectsManager.getMonitorObject("content")->getMetadataMap().at("aaa"), "bbb");
}

BOOST_AUTO_TEST_CASE(drawOptions_test)
{
  Config config;
  config.taskName = "test";
  config.consulUrl = "";
  ObjectsManager objectsManager(config.taskName, config.taskClass, config.detectorName, 0);

  TH1F h("histo", "h", 100, 0, 99);
  objectsManager.startPublishing(&h, PublicationPolicy::Forever);

  BOOST_CHECK_THROW(objectsManager.getMonitorObject("histo")->getMetadataMap().at(ObjectsManager::gDrawOptionsKey), out_of_range);
  objectsManager.setDefaultDrawOptions(&h, "colz");
  BOOST_CHECK_EQUAL(objectsManager.getMonitorObject("histo")->getMetadataMap().at(ObjectsManager::gDrawOptionsKey), "colz");
  objectsManager.setDefaultDrawOptions("histo", "alp lego1");
  BOOST_CHECK_EQUAL(objectsManager.getMonitorObject("histo")->getMetadataMap().at(ObjectsManager::gDrawOptionsKey), "alp lego1");

  BOOST_CHECK_THROW(objectsManager.getMonitorObject("histo")->getMetadataMap().at(ObjectsManager::gDisplayHintsKey), out_of_range);
  objectsManager.setDisplayHint(&h, "logx");
  BOOST_CHECK_EQUAL(objectsManager.getMonitorObject("histo")->getMetadataMap().at(ObjectsManager::gDisplayHintsKey), "logx");
  objectsManager.setDisplayHint("histo", "gridy logy");
  BOOST_CHECK_EQUAL(objectsManager.getMonitorObject("histo")->getMetadataMap().at(ObjectsManager::gDisplayHintsKey), "gridy logy");
}

BOOST_AUTO_TEST_CASE(feed_with_nullptr)
{
  Config config;
  config.taskName = "test";
  config.consulUrl = "";
  ObjectsManager objectsManager(config.taskName, config.taskClass, config.detectorName, 0);

  BOOST_CHECK_NO_THROW(objectsManager.startPublishing<true>(nullptr, PublicationPolicy::Forever));
  BOOST_CHECK_NO_THROW(objectsManager.setDefaultDrawOptions(nullptr, ""));
  BOOST_CHECK_NO_THROW(objectsManager.setDisplayHint(nullptr, ""));
  BOOST_CHECK_NO_THROW(objectsManager.stopPublishing(nullptr));
}

} // namespace o2::quality_control::core
