#pragma once

#include <QtCore>
#include <vector>

class Topology : public QObject {
    Q_OBJECT
public:
    typedef std::vector<uint64_t> Path;
    Path computePath(uint64_t sw1, uint64_t sw2);

public slots:
    void linkUp(uint64_t sw1, uint64_t sw2);
    void linkDown(uint64_t sw1, uint64_t sw2);
};
