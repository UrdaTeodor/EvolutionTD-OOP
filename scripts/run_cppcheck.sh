#!/usr/bin/bash

mkdir -p cppcheck-scan-dir

# unusedFunction suprimat global: pe un proiect SFML spart in multe fisiere
# (scene + rendereri), cppcheck nu reuseste sa urmareasca apelurile intre
# unitatile de traducere si raporteaza ~toate metodele publice ca "never used"
# (render*, hitTest, etc. — clar folosite din GameScene). Zeci de fals-pozitive,
# zero reale => suprimam check-ul (ca missingIncludeSystem/useStlAlgorithm),
# nu adaugam zeci de // cppcheck-suppress prin cod.
cppcheck --enable=all \
    --inline-suppr \
    --project="${BUILD_DIR:-build}"/compile_commands.json \
    -i"${FETCHCONTENT_BASE_DIR:-build/_deps}" --suppress="*:${FETCHCONTENT_BASE_DIR:-build/_deps}/*" \
    -i"${BUILD_DIR:-build}" --suppress="*:${BUILD_DIR:-build}/*" \
    -i"${EXT_DIR:-ext}" --suppress="*:${EXT_DIR:-ext}/*" \
    --suppress=missingIncludeSystem \
    --suppress=unmatchedSuppression \
    --suppress=useStlAlgorithm \
    --suppress=unusedFunction \
    --check-level=exhaustive \
    -j 1 \
    --cppcheck-build-dir=cppcheck-scan-dir \
    --error-exitcode=1
