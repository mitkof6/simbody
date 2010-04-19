#ifndef SimTK_SIMBODY_CONTACT_IMPL_H_
#define SimTK_SIMBODY_CONTACT_IMPL_H_

/* -------------------------------------------------------------------------- *
 *                      SimTK Core: SimTK Simbody(tm)                         *
 * -------------------------------------------------------------------------- *
 * This is part of the SimTK Core biosimulation toolkit originating from      *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org.               *
 *                                                                            *
 * Portions copyright (c) 2008-10 Stanford University and the Authors.        *
 * Authors: Peter Eastman                                                     *
 * Contributors: Michael Sherman                                              *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,    *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR      *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE  *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                     *
 * -------------------------------------------------------------------------- */

#include "simbody/internal/Contact.h"

namespace SimTK {


//==============================================================================
//                                CONTACT IMPL
//==============================================================================
/** This is the internal implementation base class for Contact. **/
class ContactImpl {
public:
    ContactImpl::ContactImpl(ContactSurfaceIndex surf1, 
                             ContactSurfaceIndex surf2,
                             Contact::Condition  condition=Contact::Unknown) 
    :   m_referenceCount(0), m_condition(condition), 
        m_id(), m_surf1(surf1), m_surf2(surf2) {}

    void setCondition(Contact::Condition cond) {m_condition=cond;}
    Contact::Condition getCondition() const {return m_condition;}

    void setContactId(ContactId id) {m_id=id;}
    ContactId getContactId() const {return m_id;}

    virtual ~ContactImpl() {
        assert(m_referenceCount == 0);
    }
    virtual ContactTypeId getTypeId() const = 0;

    /* Create a new ContactTypeId and return this unique small integer 
    (thread safe). Each distinct type of Contact should use this to
    initialize a static variable for that concrete class. */
    static ContactTypeId  createNewContactTypeId()
    {   static AtomicInteger nextAvailableId = 1;
        return ContactTypeId(nextAvailableId++); }


    /* Create a new ContactId and return this unique integer 
    (thread safe). Each distinct type of Contact should use this to
    initialize a static variable for that concrete class. This will
    roll over at approximately 1 billion. */
    static ContactId  createNewContactId()
    {   static AtomicInteger nextAvailableId = 1;
        const int MaxContactId = 999999999; // 1 billion-1
        const int id = nextAvailableId++;
        // Other threads might get a few more high-numbered ids here before
        // we reset the next available to 1, but since only one thread gets
        // exactly MaxContactId as its id, only one will execute the reset.
        if (id == MaxContactId)
            nextAvailableId = 1;
        return ContactId(id); }

protected:
friend class Contact;

    mutable int         m_referenceCount;
    Contact::Condition  m_condition;
    ContactId           m_id;
    ContactSurfaceIndex m_surf1,
                        m_surf2;
};



//==============================================================================
//                          UNTRACKED CONTACT IMPL
//==============================================================================
/** This is the internal implementation class for UntrackedContact. **/
class UntrackedContactImpl : public ContactImpl {
public:
    UntrackedContactImpl(ContactSurfaceIndex surf1, ContactSurfaceIndex surf2) 
    :   ContactImpl(surf1, surf2, Contact::Untracked) {}

    ContactTypeId getTypeId() const {return classTypeId();}
    static ContactTypeId classTypeId() {
        static const ContactTypeId tid = createNewContactTypeId();
        return tid;
    }
};


//==============================================================================
//                           BROKEN CONTACT IMPL
//==============================================================================
/** This is the internal implementation class for BrokenContact. **/
class BrokenContactImpl : public ContactImpl {
public:
    BrokenContactImpl
       (ContactSurfaceIndex surf1, ContactSurfaceIndex surf2, Real separation) 
    : ContactImpl(surf1, surf2), separation(separation) {}

    ContactTypeId getTypeId() const {return classTypeId();}
    static ContactTypeId classTypeId() {
        static const ContactTypeId tid = createNewContactTypeId();
        return tid;
    }

private:
friend class BrokenContact;
    Real        separation;
};



//==============================================================================
//                        CIRCULAR POINT CONTACT IMPL
//==============================================================================
/** This is the internal implementation class for CircularPointContact. **/
class CircularPointContactImpl : public ContactImpl {
public:
    CircularPointContactImpl
       (ContactSurfaceIndex surf1, Real radius1, 
        ContactSurfaceIndex surf2, Real radius2, 
        Real radiusEff, Real depth, const Vec3& origin, const UnitVec3& normal)
    :   ContactImpl(surf1, surf2), 
        radius1(radius1), radius2(radius2), radiusEff(radiusEff), 
        depth(depth), origin_G(origin), normal_G(normal) {}

    ContactTypeId getTypeId() const {return classTypeId();}
    static ContactTypeId classTypeId() {
        static const ContactTypeId tid = createNewContactTypeId();
        return tid;
    }

private:
friend class CircularPointContact;
    Real        radius1, radius2, radiusEff, depth;
    Vec3        origin_G;
    UnitVec3    normal_G;
};



//==============================================================================
//                            TRIANGLE MESH IMPL
//==============================================================================
/** This is the internal implementation class for TriangleMeshContact. **/
class TriangleMeshContactImpl : public ContactImpl {
public:
    TriangleMeshContactImpl(ContactSurfaceIndex surf1, 
                            ContactSurfaceIndex surf2,
                            const std::set<int>& faces1, 
                            const std::set<int>& faces2);

    ContactTypeId getTypeId() const {return classTypeId();}
    static ContactTypeId classTypeId() {
        static const ContactTypeId tid = createNewContactTypeId();
        return tid;
    }

private:
friend class TriangleMeshContact;

    const std::set<int> faces1;
    const std::set<int> faces2;
};



//==============================================================================
//                        POINT CONTACT IMPL (OBSOLETE)
//==============================================================================
/** This is the internal implementation class for PointContact. **/
class PointContactImpl : public ContactImpl {
public:
    PointContactImpl(ContactSurfaceIndex surf1, ContactSurfaceIndex surf2, 
                     Vec3& location, Vec3& normal, Real radius, Real depth);

    ContactTypeId getTypeId() const {return classTypeId();}
    static ContactTypeId classTypeId() {
        static const ContactTypeId tid = createNewContactTypeId();
        return tid;
    }

private:
friend class PointContact;

    Vec3 location, normal;
    Real radius, depth;
};



} // namespace SimTK

#endif // SimTK_SIMBODY_CONTACT_IMPL_H_
