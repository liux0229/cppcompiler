#pragma once
#include "common.h"
#include "StorageClass.h"
#include "Type.h"
#include "Initializer.h"

namespace compiler {

class Namespace;

enum class MemberKind {
  Namespace = 0x1,
  Variable = 0x2,
  Function = 0x4,
  Typedef = 0x8,
};

struct NamespaceMember;
struct VariableMember;
struct FunctionMember;
struct TypedefMember;
MakeShared(NamespaceMember);
MakeShared(VariableMember);
MakeShared(FunctionMember);
MakeShared(TypedefMember);

struct Member : std::enable_shared_from_this<Member> {
  Member(Namespace* o, 
         const std::string& n, 
         Linkage link, 
         bool defined) 
    : owner(o), name(n), linkage(link), isDefined(defined) { }

  virtual MemberKind getKind() const = 0;
  virtual void output(std::ostream& out) const = 0;

  bool isVariable() const {
    return getKind() == MemberKind::Variable;
  }
  bool isFunction() const {
    return getKind() == MemberKind::Function;
  }
  bool isTypedef() const {
    return getKind() == MemberKind::Typedef;
  }
  bool isNamespace() const {
    return getKind() == MemberKind::Namespace;
  }

  SVariableMember toVariable() {
    CHECK(isVariable());
    return std::static_pointer_cast<VariableMember>(shared_from_this());
  }
  SFunctionMember toFunction() {
    CHECK(isFunction());
    return std::static_pointer_cast<FunctionMember>(shared_from_this());
  }
  STypedefMember toTypedef() {
    CHECK(isTypedef());
    return std::static_pointer_cast<TypedefMember>(shared_from_this());
  }
  SNamespaceMember toNamespace() {
    CHECK(isNamespace());
    return std::static_pointer_cast<NamespaceMember>(shared_from_this());
  }

  bool ownedBy(const Namespace* n) const {
    return n == owner;
  }

  std::string getQualifiedName() const;

  Namespace* owner;
  const std::string name;
  // TODO: revisit this design later (does linkage naturally apply to every
  // kind of member)
  Linkage linkage;
  bool isDefined;
};
MakeShared(Member);

std::ostream& operator<<(std::ostream& out, const Member& m);

std::ostream& operator<<(std::ostream& out, SMember m);

struct TypedefMember : Member {
  TypedefMember(Namespace* owner, const std::string& name, SType t) 
    : Member(owner, 
             name, 
             Linkage::Internal, 
             true), 
      type(t) { }

  MemberKind getKind() const override { return MemberKind::Typedef; }
  void output(std::ostream& out) const override;

  SType type;
};

struct VariableMember : Member {
  VariableMember(Namespace* owner, 
                 const std::string& name, 
                 SType t,
                 Linkage link,
                 StorageDuration store,
                 bool isDef,
                 UInitializer&& init)
    : Member(owner, 
             name, 
             link,
             isDef), 
      type(t),
      storage(store),
      initializer(move(init)) { 
    if (isDefined) {
      CHECK(initializer);
    }
  }
  MemberKind getKind() const override { return MemberKind::Variable; }
  void output(std::ostream& out) const override;

  SType type;
  StorageDuration storage;
  UInitializer initializer;
};

struct FunctionMember : Member {
  FunctionMember(Namespace* owner, 
                 const std::string& name, 
                 SFunctionType t,
                 Linkage link,
                 bool inLine,
                 bool isDef) 
    : Member(owner, 
             name, 
             link,
             isDef), 
      type(t),
      isInline(inLine) { }
  MemberKind getKind() const override { return MemberKind::Function; }
  void output(std::ostream& out) const override;

  SFunctionType type;
  bool isInline;
};

struct NamespaceMember : Member {
  NamespaceMember(Namespace* owner, 
                  const std::string& name, 
                  Linkage link,
                  Namespace* n) 
    : Member(owner, name, link, true), ns(n) { }
  MemberKind getKind() const override { return MemberKind::Namespace; }

  // the member is found in the 'enclosing' namespace, with 'name'
  bool isNamespaceAlias(Namespace* enclosing, 
                        const std::string& memberName) const;

  void output(std::ostream& out) const override;

  Namespace* ns;
};

}
