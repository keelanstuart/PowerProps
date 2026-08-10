# PowerProps

*"Every value can tell you what it means."*

PowerProps is a lightweight C++ runtime property system for making application state **discoverable**.

Instead of teaching every editor, serializer, plugin, configuration system, and tool how to understand every piece of data in your application, describe that data once.

Everything else can discover it at runtime.

---

## The Idea

Applications are full of values that mean something.

A filename isn't just a string.

A position isn't just three floats.

An output format isn't just an integer.

A checkbox isn't just a boolean.

The code that owns those values knows what they mean, but the rest of the application usually doesn't. Editors need custom code to expose them. Serializers need custom code to preserve them. Plugins need custom interfaces to find them.

PowerProps gives that information a common language.

A property has a name, a type, a value, and an identity. It can also carry additional information describing what that value **means** - an aspect.

Systems that understand properties can work with those values without having been written specifically for the code that provides them.

> **Describe the state once. Let everything else discover it.**

---

## Why?

Most mature applications eventually build variations of the same machinery:

* runtime inspection
* serialization
* configuration
* property editing
* scripting interfaces
* plugin interfaces
* debugging and diagnostic tools

Usually each system gets its own way of finding and manipulating application state.

PowerProps approaches the problem from the opposite direction.

Instead of teaching every consumer how to understand every provider, give application state a common vocabulary.

The provider describes what is available.

The consumer decides what to do with it.

---

## Creating a property set

Properties live inside an `IPropertySet`.

Create one like this:

```cpp
props::IPropertySet* properties =
    props::IPropertySet::CreatePropertySet();
```

A property set is simply a discoverable collection of related properties.

For example, a packaging configuration might expose:

```text
Package
    ├── OutputPath
    ├── CompressionLevel
    ├── GenerateManifest
    └── PackageName
```

The code consuming that set does not need to know what subsystem created it.

It can enumerate everything that is available:

```cpp
for (size_t i = 0;
     i < properties->GetPropertyCount();
     i++)
{
    props::IProperty* property =
        properties->GetProperty(i);

    // Inspect its name, ID, type,
    // aspect, flags, value...
}
```

Or retrieve an individual property by name:

```cpp
props::IProperty* compression =
    properties->GetPropertyByName(
        _T("CompressionLevel"));
```

by ID:

```cpp
props::IProperty* compression =
    properties->GetPropertyById('COMP');
```

or through the indexing interface:

```cpp
props::IProperty* compression =
    (*properties)['COMP'];
```

The property set provides the common point of discovery.

---

## Creating ordinary properties

Create a property through its property set:

```cpp
props::IProperty* compression =
    properties->CreateProperty(
        _T("CompressionLevel"),
        'COMP');
```

Then give it a value:

```cpp
compression->SetInt(5);
```

The property now knows that it contains an integer.

Its value can be read later with:

```cpp
int64_t level =
    compression->AsInt();
```

A string property works the same way:

```cpp
props::IProperty* output_path =
    properties->CreateProperty(
        _T("OutputPath"),
        'PATH');

output_path->SetString(
    _T("C:\\Build\\Output"));
```

And a boolean:

```cpp
props::IProperty* manifest =
    properties->CreateProperty(
        _T("GenerateManifest"),
        'MNFT');

manifest->SetBool(true);
```

The values themselves aren't unusual.

What PowerProps adds is a common way to discover what they are, identify them, inspect them, and work with them generically.

---

## Type tells you what it is. Aspect tells you what it means.

Sometimes knowing the data type isn't enough.

Consider this:

```cpp
props::IProperty* output_file =
    properties->CreateProperty(
        _T("OutputFile"),
        'OUTF');

output_file->SetString(
    _T("installer.exe"));
```

PowerProps knows that `OutputFile` is a string.

But a generic editor still doesn't know what kind of string it represents.

Add an aspect:

```cpp
output_file->SetAspect(
    props::IProperty::PA_FILENAME);
```

Now a consumer can know that this isn't merely arbitrary text.

An editor could present a file browser.

A directory works the same way:

```cpp
output_path->SetAspect(
    props::IProperty::PA_DIRECTORY);
```

A float can describe a percentage:

```cpp
props::IProperty* progress =
    properties->CreateProperty(
        _T("Progress"),
        'PRGS');

progress->SetFloat(75.0f);

progress->SetAspect(
    props::IProperty::PA_PERCENTAGE);
```

A vector can represent a color:

```cpp
props::IProperty* tint =
    properties->CreateProperty(
        _T("Tint"),
        'TINT');

tint->SetVec3F(
    props::TVec3F(
        1.0f,
        0.5f,
        0.25f));

tint->SetAspect(
    props::IProperty::PA_COLOR_RGB);
```

Or that same underlying vector type might represent latitude and longitude, a rotation, eye position, sun direction, dimensions, or something entirely different.

The property type says:

> **What is this data?**

The aspect says:

> **What does this data mean?**

That distinction allows generic tools to make much better decisions about values they have never seen before.

---

## Properties can reference existing data

PowerProps does not require application state to live inside the property system.

A reference property can expose data that already exists somewhere else.

For example:

```cpp
props::TVec3F model_position;
props::TVec3F model_orientation;
props::TVec3F model_scale(
    1.0f,
    1.0f,
    1.0f);
```

Expose those values directly:

```cpp
properties->CreateReferenceProperty(
    _T("ModelPosition"),
    'POSN',
    &model_position,
    props::IProperty::PT_FLOAT_V3);

properties->CreateReferenceProperty(
    _T("ModelOrientation"),
    'ORNT',
    &model_orientation,
    props::IProperty::PT_FLOAT_V3);

properties->CreateReferenceProperty(
    _T("ModelScale"),
    'SCAL',
    &model_scale,
    props::IProperty::PT_FLOAT_V3);
```

Those properties do not contain duplicate copies of the values.

They refer directly to the existing application state.

If an editor changes `"ModelPosition"`, it changes `model_position`.

If application code changes `model_position`, the property immediately reflects the new value.

There is nothing to synchronize.

This makes reference properties particularly useful when adding discoverability to an existing system without reorganizing where its state lives.

---

## Configuration is just as natural

Properties don't have to describe an "object."

Consider an application with packaging settings:

```text
OutputPath
CompressionLevel
GenerateManifest
PackageName
```

A property set can describe them:

```cpp
props::IPropertySet* package =
    props::IPropertySet::CreatePropertySet();

props::IProperty* name =
    package->CreateProperty(
        _T("PackageName"),
        'NAME');

name->SetString(
    _T("MyApplication"));

props::IProperty* output =
    package->CreateProperty(
        _T("OutputPath"),
        'PATH');

output->SetString(
    _T("C:\\Build\\Output"));

output->SetAspect(
    props::IProperty::PA_DIRECTORY);

props::IProperty* compression =
    package->CreateProperty(
        _T("CompressionLevel"),
        'COMP');

compression->SetInt(5);

props::IProperty* manifest =
    package->CreateProperty(
        _T("GenerateManifest"),
        'MNFT');

manifest->SetBool(true);
```

A configuration editor doesn't need to know anything about packaging.

It asks what properties are available.

PowerProps tells it.

The editor handles the types and aspects it understands and presents them appropriately.

The same property set might later be consumed by a serializer, scripting system, plugin, command-line tool, or something that did not exist when the properties were originally defined.

---

## One description, many consumers

Imagine an application exposes:

```text
OutputPath          string / directory
CompressionLevel    integer
GenerateManifest    boolean
PackageName         string
```

An editor might turn those properties into:

```text
Output Path:        [ C:\Build\Output       ] [...]
Compression Level:  [ 5                     ]
Generate Manifest:  [✓]
Package Name:       [ MyApplication         ]
```

A serializer could preserve the same information.

A plugin could inspect it programmatically.

A debugging tool could display it.

A scripting layer could expose it.

None of those systems need a special interface for packaging.

They only need to understand properties.

---

## Generic discovery

A consumer can inspect a property set without knowing ahead of time what it contains:

```cpp
for (size_t i = 0;
     i < package->GetPropertyCount();
     i++)
{
    props::IProperty* property =
        package->GetProperty(i);

    const TCHAR* name =
        property->GetName();

    props::FOURCHARCODE id =
        property->GetID();

    props::IProperty::PROPERTY_TYPE type =
        property->GetType();

    props::IProperty::PROPERTY_ASPECT aspect =
        property->GetAspect();

    // Decide what to do based on what
    // the property says about itself.
}
```

The consuming code doesn't ask:

> "Is this a packaging configuration?"

It asks:

> "What properties are here?"

That is the relationship PowerProps is intended to create.

---

## Discoverability changes the architecture

Without a common property system:

```text
                ┌── Editor-specific interface
Application ────┼── Serializer-specific interface
   State        ├── Plugin-specific interface
                ├── Scripting-specific interface
                └── Configuration-specific interface
```

Every new consumer needs another way into the application.

With PowerProps:

```text
                        ┌── Editor
                        ├── Serializer
Application ──►         ├── Plugin
 Property Set           ├── Configuration
                        ├── Scripting
                        ├── Diagnostics
                        └── Whatever comes next
```

The producer describes its state once.

Consumers discover what is available.

---

## Build tools that don't know what's coming

This is where PowerProps becomes particularly useful.

Suppose you write a property editor today.

Next year, you add an entirely new subsystem containing properties that didn't exist when the editor was written.

If the editor understands those property types and aspects, it may require **no changes at all**.

The new state is simply discovered at runtime.

The same applies to serializers, plugin systems, scripting layers, diagnostic tools, and other generic consumers.

That's the point of PowerProps.

Not merely accessing values.

**Discovering capabilities that weren't known when the consuming code was written.**

---

## Properties can describe UI intent

PowerProps flags and aspects provide hints that generic user interfaces can use without restricting what application code itself is allowed to do.

A property can indicate that it is:

* required
* read-only
* hidden
* intended for a tooltip
* type-locked
* aspect-locked

Aspects can communicate richer meaning such as:

* filename or directory
* color
* percentage
* date or time
* latitude / longitude
* rotation
* dimensions
* matrices
* eye or lighting information
* boolean presentation such as Yes/No, On/Off, or Enabled/Disabled

The property doesn't dictate how a UI must look.

It simply provides enough meaning for a generic UI to make an informed choice.

---

## Enumerations

Properties can also represent enumerated values.

Enumeration strings can be provided directly:

```cpp
props::IProperty* quality =
    properties->CreateProperty(
        _T("Quality"),
        'QUAL');

quality->SetEnumStrings(
    _T("Low,Medium,High,Ultra"));

quality->SetEnumVal(2);
```

Or an application can implement `IEnumProvider` when the available choices need to be determined dynamically.

The consumer still sees a property.

The source of its choices is an implementation detail.

---

## Serialization

A property set can serialize itself without requiring every consumer to understand the data that produced it.

Binary serialization supports several levels of description:

```text
Values Only
    ID, type, value

Terse
    ID, type, aspect, value

Verbose
    Name, ID, type, aspect, value
```

For example:

```cpp
size_t required = 0;

package->Serialize(
    props::IProperty::SM_BIN_VERBOSE,
    nullptr,
    0,
    &required);
```

The property set can also serialize to and deserialize from XML.

This means the same property description used for editors and plugins can also serve as the basis for persistence or interchange.

---

## Property change notifications

A system that needs to react when properties change can implement `IPropertyChangeListener`:

```cpp
class PropertyListener :
    public props::IPropertyChangeListener
{
public:

    void PropertyChanged(
        const props::IProperty* property) override
    {
        // React to the change.
    }
};
```

and register it with the set:

```cpp
PropertyListener listener;

properties->SetChangeListener(
    &listener);
```

The provider can expose its state while interested systems react to changes through the same property model.

---

## Design Philosophy

PowerProps favors practical abstractions over clever ones.

There is no code generation.

No language extension.

No compiler magic.

No requirement that application state be reorganized around the property system.

Your data remains your data.

Your C++ remains ordinary C++.

PowerProps provides a small collection of interfaces and a common vocabulary through which otherwise independent parts of an application can understand one another.

---

## A Little Magic

There is an old belief that names have power.

A value without a name is merely data.

Give it a name, a type, an identity, and meaning, and other systems can begin to reason about it.

Editors can edit it.

Serializers can preserve it.

Plugins can discover it.

Tools that didn't exist when the property was written can understand it.

The data itself hasn't changed.

It has simply learned how to introduce itself.

---

## Releasing a property set

When a property set is no longer needed:

```cpp
package->Release();
```

The property set owns the ordinary properties created through it and manages their lifetime.

---

## License

MIT License.

Use it freely.

Build useful things.

Build impossible things.
