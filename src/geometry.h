//
// Created by qm210 on 19.09.2025.
//

#ifndef DLTROPHY_QMAND_GEOMETRY_H
#define DLTROPHY_QMAND_GEOMETRY_H

struct Size {
    int width = 0;
    int height = 0;

    explicit operator bool() const {
        return width > 0 && height > 0;
    }

    [[nodiscard]]
    int area() const { return width * height; }
};

#endif //DLTROPHY_QMAND_GEOMETRY_H
