# PowerProps

*"Every object knows more than it reveals."*

PowerProps is a lightweight C++ runtime property system for applications that need their objects to be *discoverable*.

Instead of teaching every editor, serializer, plugin, and configuration system how to understand every class in your application, teach your objects how to describe themselves once.

Everything else follows naturally.

---

## The Idea

Every object carries knowledge.

Its name.

Its state.

Its meaning.

Normally, that knowledge is trapped inside the object itself. Editors cannot edit it. Serializers cannot preserve it. Tools cannot inspect it. Other systems cannot reason about it without writing another layer of code.

PowerProps gives objects a common language.

A property is more than a value. It is a named piece of knowledge with type, identity, and meaning. Once an object exposes its properties, the rest of your application no longer needs to know *what* it is talking to—only that it speaks the language of properties.

---

## Why?

Most mature applications eventually build the same machinery:

* Runtime inspection
* Serialization
* Configuration
* Property editing

PowerProps approaches the problem from the opposite direction.

Instead of solving each of those independently, it asks a simpler question:

**"What if every object could simply describe itself?"**

---

## Design Philosophy

PowerProps favors practical abstractions over clever ones.

There is no code generation or compiler magic... just ordinary C++, a small collection
of interfaces, and a vocabulary that lets independent systems understand one another.

---

## A Little Magic

There is an old belief that names have power.

A thing that cannot be named cannot be reasoned about.
A thing that cannot describe itself remains unknowable.

PowerProps teaches objects to speak.

Once they do, editors may edit them, serializers may preserve them, plugins may extend them, and entirely new tools may discover capabilities they were never explicitly written to understand.

The object itself has not changed.

It has merely learned its true name.

---

## License

MIT License.

Use it freely.

Build useful things.

Build impossible things.
