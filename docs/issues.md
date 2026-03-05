# Phograph Issues

## Layout / Canvas

1. Canvas overlaps sidebar -- graph nodes render behind the left Browser pane
   instead of being clipped to the canvas area. The canvas origin or clip rect
   is wrong; nodes at low X values slide under the sidebar.

2. Graph extends past right edge -- no horizontal containment. Long graphs
   (many columns at the same depth) run off-screen with no scroll.

3. No vertical scroll -- if a graph has many rows it runs off the bottom.
   Need a scroll view (or at minimum pan+zoom) so the full graph is reachable.

## Printing / PDF Export

4. PDF export should target standard paper sizes (US Letter / A4) and paginate
   across multiple sheets when the graph is larger than one page. Currently it
   writes a single page sized to the bounding box, which may be huge.

## Class Browser / OOP

5. No classes or objects visible in the Browser -- the sidebar shows only
   "Main", "Universal Methods", and "Libraries". For a language built on
   Prograph (which is fundamentally OOP), there should be a visible class
   hierarchy: Classes > Attributes + Methods. Users should be able to create
   and browse classes, define attributes, and attach methods to them.

6. "Methods" label with no "Classes" label -- the Browser tree only has a
   "Universal Methods" section. There is no section for class-scoped methods,
   instance generators, get/set nodes, or inheritance. This makes the OOP
   features from Lessons 7+ invisible and inaccessible from the UI.

7. No instance generator or get/set creation UI -- even though the engine and
   lessons describe instance generators, get, and set nodes, there is no
   way to insert them from the canvas context menu or node picker.

## Packages / Namespaces

8. No package or namespace management -- Prograph had "Sections" that grouped
   related classes and methods. Phograph has sections in the JSON model
   (ProjectModel) but the Browser only shows the first section's contents
   flat. There is no UI to create, rename, or switch between sections.

9. No import/export of sections -- if sections are the package unit, users
   should be able to import a section from another project file or export
   one for reuse. This would complement the library system.

10. Namespace collisions -- with libraries adding primitives into a flat
    namespace, there is no mechanism to disambiguate if two libraries define
    a primitive with the same name. Sections or a dotted-name convention
    (e.g., math/sin vs sound/sin) would help.

## Documentation / Repo

11. ~~README is sparse~~ -- **Resolved.** README now has prerequisites,
    quick-start instructions, architecture overview, full test list,
    library docs, and contribution guidelines.

12. GitHub About box is empty -- repo description, topics, and homepage URL
    should be set so the project is discoverable and has a proper summary.
